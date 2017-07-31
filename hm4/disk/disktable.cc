#include "disktable.h"

#include "pairblob.h"

#include "binarysearch.h"
#include "myalign.h"
#include "levelorderlookup.h"

#include "disk/filenames.h"
#include "disk/btreeindexnode.h"

#define log__(...) /* nada */
//#include "logger.h"


namespace hm4{
namespace disk{

constexpr LevelOrderLookup<btree::NODE_LEVELS> LL;

const int DiskTable::DEFAULT_ADVICE = MMAPFile::RANDOM;

bool DiskTable::open(const std::string &filename, int advice){
	metadata_.open(filenameMeta(filename));

	if (metadata_ == false)
		return false;

	// avoid worst case
	if (metadata_.sorted() == false && advice == MMAPFile::SEQUENTIAL)
		advice = DEFAULT_ADVICE;

	bool const b1 =	mIndx_.open(filenameIndx(filename));
	bool const b2 =	mData_.open(filenameData(filename), advice);

	mTree_.open(filenameBTreeIndx(filename));
	mKeys_.open(filenameBTreeData(filename));

	return b1 && b2;
}

void DiskTable::close(){
	mIndx_.close();
	mData_.close();

	mTree_.close();
	mKeys_.close();
}

// ==============================

inline void DiskTable::openFile__(MMAPFile &file, BlobRef &blob, const StringRef &filename, int const advice){
	file.open(filename, advice);
	blob = { file.mem(), file.size() };
}

inline void DiskTable::closeFile__(MMAPFile &file, BlobRef &blob){
	blob.reset();
	file.close();
}

// ==============================

auto DiskTable::btreeSearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	using Node			= btree::Node;
	using NodeData			= btree::NodeData;

	using level_type		= btree::level_type;

	level_type const VALUES		= btree::VALUES;
	level_type const BRANCHES	= btree::BRANCHES;

	size_t const nodesCount	= mTree_.size() / sizeof(Node);

	const Node *nodes = mTree_->as<const Node>(0, nodesCount);

	if (!nodes){
		// go try with binary search
		log__("Problem, switch to binary search (1)");
		return binarySearch_(key);
	}

	std::pair<size_type, size_type> bs{ 0, size() };

	size_t pos = 0;

	log__("BTREE at ", pos);

	while(pos < nodesCount){
		const Node &node = nodes[pos];

		// MODIFIED LEVEL ORDERED MINI-BINARY SEARCH INSIDE BTREE NODE
		// OUTPUT PARAMETERS

		level_type node_index;

		// MODIFIED LEVEL ORDERED MINI-BINARY SEARCH INSIDE BTREE NODE
		// CODE
		{
			const auto &ll = LL;

			level_type node_pos = 0;

			do{

				// ACCESS ELEMENT
				// ---
				uint64_t const offset = be64toh(node.values[ node_pos ]);

				#if 0
				// BTree NIL - can not happen

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

				const NodeData *nd = mKeys_->as<const NodeData>((size_t) offset);

				if (!nd){
					// go try with binary search
					log__("Problem, switch to binary search (2)");
					return binarySearch_(key);
				}

				size_t    const keysize = be16toh(nd->keysize);
				size_type const dataid  = be64toh(nd->dataid);

				// key is just after the NodeData
				const char *keyptr = mKeys_->as<const char>((size_t) offset + sizeof(NodeData), keysize);

				if (!keyptr){
					// go try with binary search
					log__("Problem, switch to binary search (3)");
					return binarySearch_(key);
				}

				const StringRef keyx{ keyptr, keysize };
				// ---
				// EO ACCESS ELEMENT

				log__("\tNode Value", node_pos, "[", ll[node_pos], "], Key:", keyx);

				int const cmp = keyx.compare(key);

				if (cmp < 0){
					// this + 1 is because,
					// we want if possible to go LEFT instead of RIGHT
					// node_index will go out of bounds,
					// this is indicator for RIGHT
					node_index = ll[node_pos] + 1;

					// go right
					node_pos = 2 * node_pos + 1 + 1;

					bs.first = dataid + 1;

					log__("\t\tR:", node_pos, "BS:", bs.first, '-', bs.second);
				}else if (cmp > 0){
					node_index = ll[node_pos];

					// go left
					node_pos = 2 * node_pos + 1;

					bs.second = dataid;

					log__("\t\tL:", node_pos, "BS:", bs.first, '-', bs.second);
				}else{
					// found

					log__("\t\tFound at ", node_pos);

					return { true, dataid };
				}
			}while (node_pos < VALUES);
		}
		// EO MODIFIED LEVEL ORDERED MINI-BINARY SEARCH INSIDE BTREE NODE

		if ( node_index == VALUES ){
			// Go Right
			pos = pos * BRANCHES + VALUES + 1;

			log__("BTREE R:", pos);
		}else{
			// Go Left
			pos = pos * BRANCHES + node_index + 1;

			log__("BTREE L:", pos, ", node branch", node_index);
		}
	}


	// leaf or similar
	// fallback to binary search :)

	log__("BTREE LEAF:", pos);
	log__("Fallback to binary search", bs.first, '-', bs.second, ", width", bs.second - bs.first);

	return binarySearch(*this, bs.first, bs.second, key, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
}

inline auto DiskTable::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	return binarySearch(*this, size_type(0), size(), key, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
}

inline auto DiskTable::search_(const StringRef &key) const -> std::pair<bool,size_type>{
	if (mTree_ && mKeys_){
		log__("btree");
		return btreeSearch_(key);
	}else{
		log__("binary");
		return binarySearch_(key);
	}
}

// ==============================

ObserverPair DiskTable::operator[](const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	const auto x = search_(key);

	return x.first ? operator[](x.second) : nullptr;
}

auto DiskTable::lowerBound(const StringRef &key) const -> Iterator{
	const auto x = search_(key);

	return Iterator(*this, x.second, sorted());
}

// ==============================

ObserverPair DiskTable::operator[](size_type const index) const{
	// precondition
	assert( index < size() );
	// eo precondition

	const PairBlob *p = getAtFD_(index);

	return Pair::observer(p);
}

int DiskTable::cmpAt(size_type const index, const StringRef &key) const{
	// precondition
	assert(!key.empty());
	// eo precondition

	const PairBlob *p = getAtFD_(index);

	// StringRef is not null terminated
	return p ? p->cmp(key.data(), key.size()) : Pair::CMP_ZERO;
}

// ==============================

const PairBlob *DiskTable::saveAccessFD_(const PairBlob *blob) const{
	if (!blob)
		return nullptr;

	// check for overrun because PairBlob is dynamic size
	bool const access = mData_->safeAccessMemory(blob, blob->bytes());

	if (access)
		return blob;

	return nullptr;
}

const PairBlob *DiskTable::getAtFD_(size_type const index) const{
	const uint64_t *be_array = mIndx_->as<const uint64_t>(0, size());

	if (!be_array)
		return nullptr;

	const uint64_t be_ptr = be_array[index];

	size_t const offset = (size_t) be64toh(be_ptr);

	const PairBlob *blob = mData_->as<const PairBlob>(offset);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

size_t DiskTable::getSizeFD__(const PairBlob *blob, bool const aligned){
	constexpr MyAlign<PairConf::ALIGN> alc;

	size_t const size = blob->bytes();

	return ! aligned ? size : alc.calc(size);
}

const PairBlob *DiskTable::getNextFD_(const PairBlob *previous) const{
	size_t size = getSizeFD__(previous, aligned());

	const char *previousC = (const char *) previous;

	const PairBlob *blob = mData_->as<const PairBlob>(previousC + size);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

// ===================================

DiskTable::Iterator::Iterator(const DiskTable &list, size_type const pos,
							bool const sorted) :
			list_(list),
			pos_(pos),
			sorted_(sorted){}

auto DiskTable::Iterator::operator*() const -> const ObserverPair &{
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


} // namespace disk
} // namespace


