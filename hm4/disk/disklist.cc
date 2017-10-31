#include "disklist.h"

#include "binarysearch.h"
#include "myalign.h"
#include "mynarrow.h"
#include "levelorderlookup.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#define log__(...) /* nada */
#include "logger.h"



namespace hm4{
namespace disk{

constexpr LevelOrderLookup<btree::NODE_LEVELS> LL;

// ==============================

bool DiskList::open(const StringRef &filename, MMAPFile::Advice advice){
	metadata_.open(filenameMeta(filename));

	if (metadata_ == false)
		return false;

	// avoid worst case
	if (metadata_.sorted() == false && advice == MMAPFile::Advice::SEQUENTIAL)
		advice = DEFAULT_ADVICE;

	bool const b1 =	mIndx_.open(filenameIndx(filename));
	bool const b2 =	mData_.open(filenameData(filename), advice);

	mTree_.open(filenameBTreeIndx(filename));
	mKeys_.open(filenameBTreeData(filename));

	return b1 && b2;
}

void DiskList::close(){
	mIndx_.close();
	mData_.close();

	mTree_.close();
	mKeys_.close();
}

// ==============================

auto DiskList::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	return binarySearch(*this, size_type(0), size(), key, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
}

auto DiskList::search_(const StringRef &key) const -> std::pair<bool,size_type>{
	if (mTree_ && mKeys_){
		log__("btree");
		return btreeSearch_(key);
	}else{
		log__("binary");
		return binarySearch_(key);
	}
}

// ==============================

const Pair *DiskList::saveAccessFD_(const Pair *blob) const{
	if (!blob)
		return nullptr;

	// check for overrun because PairBlob is dynamic size
	bool const access = mData_->safeAccessMemory(blob, blob->bytes());

	return access ? blob : nullptr;
}

const Pair *DiskList::getAtFD_(size_type const index) const{
	const uint64_t *be_array = mIndx_->as<const uint64_t>(0, narrow<size_t>(size()));

	if (!be_array)
		return nullptr;

	const uint64_t be_ptr = be_array[index];

	size_t const offset = narrow<size_t>(be64toh(be_ptr));

	const Pair *blob = mData_->as<const Pair>(offset);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

size_t DiskList::getSizeFD__(const Pair *blob, bool const aligned){
	constexpr MyAlign<PairConf::ALIGN> alc;

	size_t const size = blob->bytes();

	return ! aligned ? size : alc.calc(size);
}

const Pair *DiskList::getNextFD_(const Pair *previous) const{
	size_t size = getSizeFD__(previous, aligned());

	const char *previousC = (const char *) previous;

	const Pair *blob = mData_->as<const Pair>(previousC + size);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

// ===================================

DiskList::Iterator::Iterator(const DiskList &list, size_type const pos,
							bool const sorted) :
			list_(list),
			pos_(pos),
			sorted_(sorted){}

const Pair &DiskList::Iterator::operator*() const{
	if (tmp_blob && pos_ == tmp_pos)
		return *tmp_blob;

	const Pair *blob;

	if (sorted_ && tmp_blob && pos_ == tmp_pos + 1){
		// get data without seek, walk forward
		// this gives 50% performance
		blob = list_.getNextFD_(tmp_blob);
	}else{
		blob = list_.getAtFD_(pos_);
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

		return binarySearch(list_, start, end, key_, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
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
#endif

