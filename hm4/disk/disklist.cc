#include "disklist.h"

#include "myalign.h"
#include "levelorderlookup.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#include "smallstring.h"

#include "binarysearch.h"

//#define log__(...) /* nada */
#include "logger.h"



namespace hm4{
namespace disk{

constexpr LevelOrderLookup<btree::NODE_LEVELS> LL;

// ==============================

struct SmallNode{
	char		key[PairConf::HLINE_SIZE];
	uint64_t	pos;
} __attribute__((__packed__));

static_assert(std::is_pod<SmallNode>::value, "SmallNode must be POD type");

// ==============================

namespace{
	using iterator = DiskList::iterator;

	// -----------------------------------

	auto search_fix(BinarySearchResult<iterator> const result, DiskList const &list, std::true_type){
		return result.found ? result.it : std::end(list);
	}

	auto search_fix(BinarySearchResult<iterator> const result, DiskList const &, std::false_type){
		return result.it;
	}

	// -----------------------------------

	int comp(Pair const &p, StringRef const &key){
		return p.cmp(key);
	};

	auto searchBinary(StringRef const &key, iterator first, iterator last){
		return binarySearch(first, last, key, comp);
	}

	auto searchBinary(StringRef const &key, DiskList const &list){
		return searchBinary(key, std::begin(list), std::end(list));
	}

	// -----------------------------------

	auto searchBTree(StringRef const &key, DiskList const &list) -> BinarySearchResult<iterator>;

	template<typename T>
	auto dt(T const &a){
		return static_cast<DiskList::difference_type>(a);
	}

//	void dt(DiskList::difference_type){}

	auto searchHotLine(StringRef const &key, DiskList const &list, MMAPFilePlus const &line) -> BinarySearchResult<iterator>{
		size_t const nodesCount = line.sizeArray<SmallNode>();

		const SmallNode *nodes = line->as<const SmallNode>(0, nodesCount);

		if (!nodes)
			return searchBinary(key, list);

		auto comp = [](SmallNode const &node, StringRef const &key){
			using SS = SmallString<PairConf::HLINE_SIZE>;

			return SS::compare(node.key, key.data(), key.size());
		};

		// first try to locate the partition
		auto const nodesEnd = nodes + nodesCount;
		auto const x = binarySearch(nodes, nodesEnd, key, comp);

		if (x.it == nodesEnd){
			log__("Not found, in most right pos");

			return { false, list.end() };
		}

		auto const listPos = dt( betoh<uint64_t>(x.it->pos) );

		if ( listPos >= dt( list.size() ) ){
			log__("Hotline corruption detected. Advice Hotline removal.");

			return searchBinary(key, list);
		}

		if (x.found == false){
			log__(
				"Not found",
				"pos", listPos,
				"key",	StringRef{ x.it->key, PairConf::HLINE_SIZE }
			);

			return { false,  iterator{ list, listPos } };
		}

		// OK, is found in the hot line...

		if (key.size() < PairConf::HLINE_SIZE){
			// if key.size() == PairConf::HLINE_SIZE,
			// this does not mean that key is found...

			log__("Found, direct hit at pos", listPos);

			return { true, iterator{ list, listPos } };
		}

		// binary inside the partition

		auto listPosLast = x.it + 1 == nodesEnd ? dt(list.size()) : dt( betoh<uint64_t>( (x.it + 1)->pos) );

		log__(
			"Proceed with Binary Search", listPos, listPosLast,
			"Hotline Key",	StringRef{ x.it->key, PairConf::HLINE_SIZE }
		);

		return searchBinary(key, list.begin() + listPos, list.begin() + listPosLast);
	}

} // anonymous namespace



// ==============================



bool DiskList::openMinimal_(StringRef const &filename, MMAPFile::Advice advice){
	metadata_.open(filenameMeta(filename));

	if (metadata_ == false)
		return false;

	// avoid worst case
	if (metadata_.sorted() == false && advice == MMAPFile::Advice::SEQUENTIAL)
		advice = DEFAULT_ADVICE;

	bool const b1 =	mIndx_.open(filenameIndx(filename));
	bool const b2 =	mData_.open(filenameData(filename), advice);

	// integrity check, size is safe to be used now.
	bool const b3 =	mIndx_.sizeArray<uint64_t>() == size();

	return b1 && b2 && b3;
}

bool DiskList::openNormal_(StringRef const &filename, MMAPFile::Advice const advice){
	if (openMinimal_(filename, advice) == false)
		return false;

	// ==============================

	mLine_.open(filenameLine(filename));

	if (mLine_.sizeArray<SmallNode>() <= 1){
		log__("Hotline too small. Ignoring.");
		mLine_.close();
	}

	// ==============================

	mTree_.open(filenameBTreeIndx(filename));
	mKeys_.open(filenameBTreeData(filename));

	return true;
}

void DiskList::close(){
	mIndx_.close();
	mData_.close();

	mLine_.close();

	mTree_.close();
	mKeys_.close();
}



// ==============================



const Pair *DiskList::fdSafeAccess_(const Pair *blob) const{
	if (!blob)
		return nullptr;

	// check for overrun because PairBlob is dynamic size
	bool const access = mData_->safeAccessMemory(blob, blob->bytes());

	return access ? blob : nullptr;
}

const Pair *DiskList::fdGetAt_(size_type const index) const{
	const uint64_t *be_array = mIndx_->as<const uint64_t>(0, narrow<size_t>(size()));

	if (!be_array)
		return nullptr;

	uint64_t const be_ptr = be_array[index];

	size_t const offset = narrow<size_t>(betoh<uint64_t>(be_ptr));

	const Pair *blob = mData_->as<const Pair>(offset);

	// check for overrun because PairBlob is dynamic size
	return fdSafeAccess_(blob);
}

#if 0
const Pair *DiskList::fdGetNext_(const Pair *previous) const{
	size_t size = alignedSize__(previous, aligned());

	const char *previousC = (const char *) previous;

	const Pair *blob = mData_->as<const Pair>(previousC + size);

	// check for overrun because PairBlob is dynamic size
	return fdSafeAccess_(blob);
}

size_t DiskList::alignedSize__(const Pair *blob, bool const aligned){
	constexpr MyAlign<PairConf::ALIGN> alc;

	size_t const size = blob->bytes();

	return ! aligned ? size : alc.calc(size);
}
#endif



// ==============================



template<bool B>
auto DiskList::search_(StringRef const &key, std::bool_constant<B> const exact) const -> iterator{
	if (mTree_ && mKeys_){
		log__("btree");
		auto const x = searchBTree(key, *this);
		return search_fix(x, *this, exact);

	}else
	if (mLine_){
		log__("hot line");
		auto const x = searchHotLine(key, *this, mLine_);
		return search_fix(x, *this, exact);

	}else
	{
		log__("binary");
		auto const x = searchBinary(key, *this);
		return search_fix(x, *this, exact );
	}
}

template auto DiskList::search_(StringRef const &key, std::true_type  ) const -> iterator;
template auto DiskList::search_(StringRef const &key, std::false_type ) const -> iterator;



// ==============================



class DiskList::BTreeSearchHelper{
private:
	using Node				= btree::Node;
	using NodeData				= btree::NodeData;

	using level_type			= btree::level_type;

	using size_type				= DiskList::size_type;

private:
	constexpr static level_type VALUES	= btree::VALUES;
	constexpr static level_type BRANCHES	= btree::BRANCHES;

private:
	class BTreeAccessError : std::exception{};

	struct NodeResult{
		bool			isResult;

		// will not fail if level_type is same as size_type
		union{
			level_type	node_index;
			size_type	result_index;
		};

	public:
		constexpr NodeResult(std::true_type,  size_type  const result_index) : isResult(true),  result_index(result_index){}
		constexpr NodeResult(std::false_type, level_type const node_index  ) : isResult(false), node_index(node_index){}
	};

	struct NodeValueResult{
		StringRef	key;
		size_type	dataid;
	};

private:
	const DiskList		&list_;

	const StringRef		&key_;

	const MMAPFilePlus	&mTree_		= list_.mTree_;
	const MMAPFilePlus	&mKeys_		= list_.mKeys_;

private:
	size_type		start		= 0;
	size_type		end		= list_.size();

public:
	BTreeSearchHelper(const DiskList &list, const StringRef &key) : list_(list), key_(key){}

	auto operator()(){
		try{
			return btreeSearch_();
		}catch(const BTreeAccessError &e){
			log__("Problem, switch to binary search (1)");
			return searchBinary(key_, list_);
		}
	}

private:
	BinarySearchResult<iterator> btreeSearch_(){
		size_t const nodesCount	= mTree_.size() / sizeof(Node);

		const Node *nodes = mTree_->as<const Node>(0, nodesCount);

		if (!nodes)
			throw BTreeAccessError{};

		size_t pos = 0;

		log__("BTREE at ", pos);

		while(pos < nodesCount){
			const auto x = levelOrderBinarySearch_( nodes[pos] );

			if (x.isResult)
				return { true, iterator{ list_, x.result_index } };

			// Determine where to go next...

			if ( x.node_index == VALUES ){
				// Go Right
				pos = pos * BRANCHES + VALUES + 1;

				log__("BTREE R:", pos);
			}else{
				// Go Left
				pos = pos * BRANCHES + x.node_index + 1;

				log__("BTREE L:", pos, ", node branch", x.node_index);
			}
		}

		// leaf or similar
		// fallback to binary search :)

		log__("BTREE LEAF:", pos);
		log__("Fallback to binary search", start, '-', end, ", width", end - start);

		return searchBinary(key_, iterator{ list_, start }, iterator{ list_, end } );
	}

	NodeResult levelOrderBinarySearch_(const Node &node){
		const auto &ll = LL;

		level_type node_pos = 0;

		level_type node_index;

		do{
			const auto x = accessNodeValue_( node.values[node_pos] );

			log__("\tNode Value", node_pos, "[", ll[node_pos], "], Key:", x.key);

			int const cmp = x.key.compare(key_);

			if (cmp < 0){
				// this + 1 is because,
				// we want if possible to go LEFT instead of RIGHT
				// node_index will go out of bounds,
				// this is indicator for RIGHT
				node_index = level_type(ll[node_pos] + 1);

				// go right
				node_pos = level_type(2 * node_pos + 1 + 1);

				start = x.dataid + 1;

				log__("\t\tR:", node_pos, "BS:", start, '-', end);
			}else if (cmp > 0){
				node_index = ll[node_pos];

				// go left
				node_pos = level_type(2 * node_pos + 1);

				end = x.dataid;

				log__("\t\tL:", node_pos, "BS:", start, '-', end);
			}else{
				// found

				log__("\t\tFound at ", node_pos);

				return { std::true_type{}, x.dataid };
			}
		}while (node_pos < VALUES);

		return { std::false_type{}, node_index };
	}

	NodeValueResult accessNodeValue_(uint64_t const offsetBE) const{
		uint64_t const offset = betoh<uint64_t>( offsetBE );

		// BTree NIL case - can not happen

		const NodeData *nd = mKeys_->as<const NodeData>(narrow<size_t>(offset));

		if (!nd)
			throw BTreeAccessError{};

		size_t    const keysize = betoh<uint16_t>(nd->keysize);
		size_type const dataid  = betoh<uint64_t>(nd->dataid);

		// key is just after the NodeData
		const char *keyptr = mKeys_->as<const char>(narrow<size_t>(offset + sizeof(NodeData)), keysize);

		if (!keyptr)
			throw BTreeAccessError{};

		return { { keyptr, keysize }, dataid };
	}
};



// ==============================



namespace{
	auto searchBTree(StringRef const &key, DiskList const &list) -> BinarySearchResult<iterator>{
		DiskList::BTreeSearchHelper helper{list, key};

		return helper();
	}
} // anonymous namespace


} // namespace disk
} // namespace

