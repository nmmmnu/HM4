#include "disktable.h"

#include "pairblob.h"

#include "binarysearch.h"

#include "disk/filenames.h"


#include "logger.h"


namespace hm4{
namespace disk{


bool DiskTable::open(const std::string &filename){
	metadata_.open(filenameMeta(filename));

	if (metadata_ == false)
		return false;

	openFile__(mmapIndx_, blobIndx_, filenameIndx(filename)	);
	openFile__(mmapData_, blobData_, filenameData(filename)	);

	return true;
}

void DiskTable::close(){
	closeFile__(mmapIndx_, blobIndx_);
	closeFile__(mmapData_, blobData_);
}

// ==============================

inline void DiskTable::openFile__(MMAPFile &file, BlobRef &blob, const StringRef &filename){
	file.open(filename);
	blob = { file.mem(), file.size() };
}

inline void DiskTable::closeFile__(MMAPFile &file, BlobRef &blob){
	blob.reset();
	file.close();
}

// ==============================

inline auto DiskTable::binarySearch_(const StringRef &key) const -> std::pair<bool,size_type>{
	return binarySearch(*this, size_type(0), size(), key, BinarySearchCompList{}, BIN_SEARCH_MINIMUM_DISTANCE);
}

inline auto DiskTable::search_(const StringRef &key) const -> std::pair<bool,size_type>{
	{
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

	return Iterator(*this, x.second, metadata_.sorted());
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
	bool const access = blobData_.safeAccessMemory(blob, blob->bytes());

	if (access)
		return blob;

	return nullptr;
}

const PairBlob *DiskTable::getAtFD_(size_type const index) const{
	const uint64_t *be_array = blobIndx_.as<const uint64_t>(0, size());

	if (!be_array)
		return nullptr;

	const uint64_t be_ptr = be_array[index];

	size_t const offset = (size_t) be64toh(be_ptr);

	const PairBlob *blob = blobData_.as<const PairBlob>(offset);

	// check for overrun because PairBlob is dynamic size
	return saveAccessFD_(blob);
}

const PairBlob *DiskTable::getNextFD_(const PairBlob *previous) const{
	size_t const size = previous->bytes( metadata_.aligned() );

	if (metadata_.aligned()){
	//	log__( "next", metadata_.aligned() ? "with align" : "");
	}

	const char *previousC = (const char *) previous;

	const PairBlob *blob = blobData_.as<const PairBlob>(previousC + size);

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

	Pair p = Pair::observer(blob);

	tmp_pos		= pos_;
	tmp_blob	= blob;
	tmp_pair 	= std::move(p);

	return tmp_pair;
}


} // namespace disk
} // namespace

