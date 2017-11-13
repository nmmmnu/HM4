#include "disklist.h"

#include "binarysearch.h"
#include "myalign.h"
#include "levelorderlookup.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#include "smallstring.h"

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



bool DiskList::open(const StringRef &filename, MMAPFile::Advice advice){
	metadata_.open(filenameMeta(filename));

	if (metadata_ == false)
		return false;

	// avoid worst case
	if (metadata_.sorted() == false && advice == MMAPFile::Advice::SEQUENTIAL)
		advice = DEFAULT_ADVICE;

	{
		bool const b1 =	mIndx_.open(filenameIndx(filename));
		bool const b2 =	mData_.open(filenameData(filename), advice);

		// integrity check, size is safe to be used now.
		bool const b3 =	mIndx_.sizeArray<uint64_t>() == size();

		if ( b1 && b2 && b3 ){
			// Ok...
		}else
			return false;
	}

	// ==============================

	mLine_.open(filenameLine(filename));

	if (mLine_.sizeArray<SmallNode>() <= 1){
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



auto DiskList::binarySearch_(const StringRef &key, size_type const start, size_type const end) const -> std::pair<bool,size_type>{
	auto comp = [](const auto &list, auto const index, const auto &key){
		return list.cmpAt(index, key);
	};

	return binarySearch(*this, start, end, key, comp);
}

auto DiskList::hotLineSearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	size_t const count =  mLine_.sizeArray<SmallNode>();

	const SmallNode *nodes = mLine_->as<const SmallNode>(0, count);

	if (!nodes)
		return binarySearch_(key);

	auto comp = [](const auto &list, auto const index, const auto &key){
		using SS = SmallString<PairConf::HLINE_SIZE>;

		return SS::compare(list[index].key, key.data(), key.size());
	};

	// first try to locate the partition
	const auto x = binarySearch(nodes, size_t{ 0 }, count, key, comp);

	if (x.second >= count){
		log__("Not found, in most right pos");

		return { false, size() };
	}

	const SmallNode &result = nodes[x.second];

	const auto pos = be64toh(result.pos);

	if ( pos >= size()){
		log__("Hotline corruption detected. Advicing Hotline removal.");
		return binarySearch_(key);
	}

	// this not work for ==,
	// because the key might actually missing from the table
	if (x.first && key.size() < PairConf::HLINE_SIZE){
		log__("Found, direct hit at pos", pos);
		return { true, pos };
	}

	if (x.first == false){
		log__(
			"Not found",
			"pos", pos,
			"key",	StringRef{ result.key, PairConf::HLINE_SIZE }
		);

		return { false, pos };
	}

	// binary inside the partition

	size_type const start = pos;
	size_type const end   = x.second == count - 1 ? size() : be64toh(nodes[x.second + 1].pos);

	log__(
		"Proceed with Binary Search", start, end,
		"key",	StringRef{ result.key, PairConf::HLINE_SIZE }
	);

	return binarySearch_(key, start, end);
}

auto DiskList::search_(const StringRef &key) const -> std::pair<bool,size_type>{
	if (mTree_ && mKeys_){
		log__("btree");
		return btreeSearch_(key);

	}else if (mLine_){
		log__("hot line");
		return hotLineSearch_(key);

	}else{
		log__("binary");
		return binarySearch_(key);
	}
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

	size_t const offset = narrow<size_t>(be64toh(be_ptr));

	const Pair *blob = mData_->as<const Pair>(offset);

	// check for overrun because PairBlob is dynamic size
	return fdSafeAccess_(blob);
}

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

// ===================================

DiskList::Iterator::Iterator(const DiskList &list, size_type const pos,
							bool const sorted) :
			list_(list),
			pos_(std::min(pos, list.size())),
			sorted_(sorted){}

const Pair &DiskList::Iterator::operator*() const{
	if (tmp_blob && pos_ == tmp_pos)
		return *tmp_blob;

	const Pair *blob;

	if (sorted_ && tmp_blob && pos_ == tmp_pos + 1){
		// get data without seek, walk forward
		// this gives 50% performance
		blob = list_.fdGetNext_(tmp_blob);
	}else{
		blob = list_.fdGetAt_(pos_);
	}

	tmp_pos		= pos_;
	tmp_blob	= blob;

	return *tmp_blob;
}





// ==============================





class DiskList::BTreeSearchHelper{
private:
	using Node				= btree::Node;
	using NodeData				= btree::NodeData;

	using level_type			= btree::level_type;

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

	std::pair<bool,size_type> operator()(){
		try{
			return btreeSearch_();
		}catch(const BTreeAccessError &e){
			log__("Problem, switch to binary search (1)");
			return binarySearchFallback_();
		}
	}

private:
	std::pair<bool,size_type> binarySearchFallback_() const{
		return list_.binarySearch_(key_);
	}

	std::pair<bool,size_type> btreeSearch_(){
		size_t const nodesCount	= mTree_.size() / sizeof(Node);

		const Node *nodes = mTree_->as<const Node>(0, nodesCount);

		if (!nodes)
			throw BTreeAccessError{};

		size_t pos = 0;

		log__("BTREE at ", pos);

		while(pos < nodesCount){
			const auto x = levelOrderBinarySearch_( nodes[pos] );

			if (x.isResult)
				return { true, x.result_index };

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

		return list_.binarySearch_(key_, start, end);
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
		uint64_t const offset = be64toh( offsetBE );

		// BTree NIL case - can not happen

		const NodeData *nd = mKeys_->as<const NodeData>(narrow<size_t>(offset));

		if (!nd)
			throw BTreeAccessError{};

		size_t    const keysize = be16toh(nd->keysize);
		size_type const dataid  = be64toh(nd->dataid);

		// key is just after the NodeData
		const char *keyptr = mKeys_->as<const char>(narrow<size_t>(offset + sizeof(NodeData)), keysize);

		if (!keyptr)
			throw BTreeAccessError{};

		return { { keyptr, keysize }, dataid };
	}
};





// ==============================





inline auto DiskList::btreeSearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	BTreeSearchHelper search(*this, key);
	return search();
}



} // namespace disk
} // namespace


// ==============================


#if 0
// BTree NIL case

if (offset == Node::NIL){
	// special case go left
	/*
	 * input tree:
	 *
	 *       n
	 *   2       n
	 * 1   n   n   n
	 *
	 * output array:
	 *
	 * n 2 n 1 n n n
	 */

	node_pos = 2 * node_pos + 1;

	log__("\t\tL:", node_pos, "NIL");

	continue;
}

const char *DiskList::fdLine_(size_type const index) const{
	struct line_type{
		char ptr[PairConf::HLINE_SIZE];
	};

	const line_type *line_array = mLine_->as<const line_type>(0, narrow<size_t>(size()));

	if (!line_array)
		return nullptr;

	return line_array[index].ptr;
}

int DiskList::cmpAt(size_type const index, const StringRef &key, cmp_line_t) const{
	assert(!key.empty());

	constexpr size_t hline_size = PairConf::HLINE_SIZE;

	const char *ss = fdLine_(index);

	if (ss){
		size_t const size = strnlen(ss, hline_size);

		int const result = - key.compare(ss, size);

		if (result || size < hline_size)
			return result;
	}

	return cmpAt(index, key, cmp_direct_t{});
}


int DiskList::cmpAt(size_type index, const StringRef &key) const{
	assert(!key.empty());

	if (mLine_)
		return cmpAt(index, key, cmp_line_t{}	);
	else
		return cmpAt(index, key, cmp_direct_t{}	);
}

#endif


