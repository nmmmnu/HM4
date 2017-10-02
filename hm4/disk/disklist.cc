#include "disklist.h"

#include "pairblob.h"

#include "binarysearch.h"
#include "myalign.h"
#include "levelorderlookup.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#define log__(...) /* nada */
#include "logger.h"


namespace hm4{
namespace disk{

constexpr LevelOrderLookup<btree::NODE_LEVELS> LL;

// ==============================

bool DiskList::open(const std::string &filename, MMAPFile::Advice advice){
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

inline auto DiskList::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	return binarySearch(*this, size_type(0), size(), key, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
}

inline auto DiskList::search_(const StringRef &key) const -> std::pair<bool,size_type>{
	if (mTree_ && mKeys_){
		log__("btree");
		return btreeSearch_(key);
	}else{
		log__("binary");
		return binarySearch_(key);
	}
}

// ==============================

ObserverPair DiskList::operator[](const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	const auto x = search_(key);

	return x.first ? operator[](x.second) : nullptr;
}

auto DiskList::lowerBound(const StringRef &key) const -> Iterator{
	const auto x = search_(key);

	return Iterator(*this, x.second, sorted());
}

// ==============================

ObserverPair DiskList::operator[](size_type const index) const{
	// precondition
	assert( index < size() );
	// eo precondition

	const PairBlob *p = getAtFD_(index);

	return Pair::observer(p);
}

int DiskList::cmpAt(size_type const index, const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	const PairBlob *p = getAtFD_(index);

	// StringRef is not null terminated
	return p ? p->cmp(key.data(), key.size()) : Pair::CMP_ZERO;
}

// ==============================

const PairBlob *DiskList::saveAccessFD_(const PairBlob *blob) const{
	if (!blob)
		return nullptr;

	// check for overrun because PairBlob is dynamic size
	bool const access = mData_->safeAccessMemory(blob, blob->bytes());

	if (access)
		return blob;

	return nullptr;
}

const PairBlob *DiskList::getAtFD_(size_type const index) const{
	const uint64_t *be_array = mIndx_->as<const uint64_t>(0, size());

	if (!be_array)
		return nullptr;

	const uint64_t be_ptr = be_array[index];

	size_t const offset = (size_t) be64toh(be_ptr);

	const PairBlob *blob = mData_->as<const PairBlob>(offset);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

size_t DiskList::getSizeFD__(const PairBlob *blob, bool const aligned){
	constexpr MyAlign<PairConf::ALIGN> alc;

	size_t const size = blob->bytes();

	return ! aligned ? size : alc.calc(size);
}

const PairBlob *DiskList::getNextFD_(const PairBlob *previous) const{
	size_t size = getSizeFD__(previous, aligned());

	const char *previousC = (const char *) previous;

	const PairBlob *blob = mData_->as<const PairBlob>(previousC + size);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

// ===================================

DiskList::Iterator::Iterator(const DiskList &list, size_type const pos,
							bool const sorted) :
			list_(list),
			pos_(pos),
			sorted_(sorted){}

auto DiskList::Iterator::operator*() const -> const ObserverPair &{
	if (tmp_blob && pos_ == tmp_pos)
		return tmp_pair;

	const PairBlob *blob;

	if (sorted_ && tmp_blob && pos_ == tmp_pos + 1){
		// get data without seek, walk forward
		// this gives 50% performance
		blob = list_.getNextFD_(tmp_blob);
	}else{
		blob = list_.getAtFD_(pos_);
	}

	tmp_pos		= pos_;
	tmp_blob	= blob;

	tmp_pair	= Pair::observer(blob);

	return tmp_pair;
}

// ==============================
// ==============================
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
	enum class SubResultType : uint8_t{
		ERROR,
		RESULT_INDEX,
		NODE_INDEX
	};

	class SubResult{
	private:
		SubResult() = default;

	public:
		constexpr static SubResult error(){
			return { SubResultType::ERROR };
		}

		constexpr static SubResult result(size_type const result_index){
			return { SubResultType::RESULT_INDEX, {}, result_index };
		}

		constexpr static SubResult node(level_type const node_index){
			return { SubResultType::NODE_INDEX, node_index, {} };
		}

	public:
		SubResultType	type;
		level_type	node_index	= 0;
		size_type	result_index	= 0;
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
		size_t const nodesCount	= mTree_.size() / sizeof(Node);

		const Node *nodes = mTree_->as<const Node>(0, nodesCount);

		if (!nodes){
			// go try with binary search
			log__("Problem, switch to binary search (1)");
			return binarySearchFallback_();
		}

		size_t pos = 0;

		log__("BTREE at ", pos);

		while(pos < nodesCount){
			const Node &node = nodes[pos];

			//    MODIFIED LEVEL ORDERED MINI-BINARY SEARCH INSIDE BTREE NODE
			const SubResult x = subSearch(node);
			// EO MODIFIED LEVEL ORDERED MINI-BINARY SEARCH INSIDE BTREE NODE

			switch(x.type){
			case SubResultType::ERROR:
				return binarySearchFallback_();

			case SubResultType::RESULT_INDEX:
				return { true, x.result_index };

			case SubResultType::NODE_INDEX:
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
		}

		// leaf or similar
		// fallback to binary search :)

		log__("BTREE LEAF:", pos);
		log__("Fallback to binary search", start, '-', end, ", width", end - start);

		return binarySearch(list_, start, end, key_, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
	}


private:
	std::pair<bool,size_type> binarySearchFallback_() const{
		return list_.binarySearch_(key_);
	}

	SubResult subSearch(const Node &node){
		const auto &ll = LL;

		level_type node_pos = 0;

		level_type node_index;

		do{

			// ACCESS ELEMENT
			// ---
			uint64_t const offset = be64toh(node.values[ node_pos ]);

			// BTree NIL case - can not happen

			const NodeData *nd = mKeys_->as<const NodeData>((size_t) offset);

			if (!nd){
				// go try with binary search
				log__("Problem, switch to binary search (2)");
				return SubResult::error();
			}

			size_t    const keysize = be16toh(nd->keysize);
			size_type const dataid  = be64toh(nd->dataid);

			// key is just after the NodeData
			const char *keyptr = mKeys_->as<const char>((size_t) offset + sizeof(NodeData), keysize);

			if (!keyptr){
				// go try with binary search
				log__("Problem, switch to binary search (3)");
				return SubResult::error();
			}

			const StringRef keyx{ keyptr, keysize };
			// ---
			// EO ACCESS ELEMENT

			log__("\tNode Value", node_pos, "[", ll[node_pos], "], Key:", keyx);

			int const cmp = keyx.compare(key_);

			if (cmp < 0){
				// this + 1 is because,
				// we want if possible to go LEFT instead of RIGHT
				// node_index will go out of bounds,
				// this is indicator for RIGHT
				node_index = level_type(ll[node_pos] + 1);

				// go right
				node_pos = level_type(2 * node_pos + 1 + 1);

				start = dataid + 1;

				log__("\t\tR:", node_pos, "BS:", start, '-', end);
			}else if (cmp > 0){
				node_index = ll[node_pos];

				// go left
				node_pos = level_type(2 * node_pos + 1);

				end = dataid;

				log__("\t\tL:", node_pos, "BS:", start, '-', end);
			}else{
				// found

				log__("\t\tFound at ", node_pos);

				return SubResult::result(dataid);
			}
		}while (node_pos < VALUES);

		return SubResult::node(node_index);
	}
};

// ==============================

auto DiskList::btreeSearch_(const StringRef &key) const -> std::pair<bool,size_type>{
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

