#ifndef _DISK_TABLE_H
#define _DISK_TABLE_H

#include "mmapfile.h"
#include "blobref.h"

#include "ilist.h"
#include "filemeta.h"

namespace hm4{
namespace disk{


class DiskTable : public List<DiskTable>{
public:
	class Iterator;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;

public:
	DiskTable() : mmapData_(MMAPFile::RANDOM){}

	DiskTable(DiskTable &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	~DiskTable() = default;

	bool open(const std::string &filename);
	void close();

	operator bool(){
		return metadata_;
	}

	void printMetadata() const{
		metadata_.print();
	}

public:
	ObserverPair operator[](const StringRef &key) const;

	ObserverPair operator[](size_type const index) const;

	int cmpAt(size_type index, const StringRef &key) const;

	size_type size() const{
		return (size_type) metadata_.size();
	}

	size_t bytes() const{
		return mmapData_.size();
	}

	bool sorted() const{
		return metadata_.sorted();
	}

	bool aligned() const{
		return metadata_.aligned();
	}

public:
	Iterator lowerBound(const StringRef &key) const;
	Iterator begin() const;
	Iterator end() const;

private:
	const PairBlob *saveAccessFD_(const PairBlob *blob) const;

	const PairBlob *getAtFD_(size_type index) const;
	const PairBlob *getNextFD_(const PairBlob *blob) const;

	static size_t getSizeFD__(const PairBlob *blob, bool aligned);

private:
	std::pair<bool,size_type> binarySearch_(const StringRef &key) const;
	std::pair<bool,size_type> btreeSearch_(const StringRef &key) const;

	std::pair<bool,size_type> search_(const StringRef &key) const;

	static void  openFile__(MMAPFile &file, BlobRef &blob, const StringRef &filename);
	static void closeFile__(MMAPFile &file, BlobRef &blob);

private:
	MMAPFile			mmapIndx_;
	BlobRef				blobIndx_;

	MMAPFile			mmapData_;
	BlobRef				blobData_;

	MMAPFile			mmapTree_;
	BlobRef				blobTree_;

	MMAPFile			mmapKeys_;
	BlobRef				blobKeys_;

	FileMeta			metadata_;
};

// ===================================

class DiskTable::Iterator{
private:
	friend class DiskTable;
	Iterator(const DiskTable &list, size_type pos,
				bool sorted_);

public:
	Iterator &operator++(){
		++pos_;
		return *this;
	}

	Iterator &operator--(){
		--pos_;
		return *this;
	}

	bool operator==(const Iterator &other) const{
		return &list_ == &other.list_ && pos_ == other.pos_;
	}

	const ObserverPair &operator*() const;

public:
	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const ObserverPair *operator ->() const{
		return & operator*();
	}

private:
	const DiskTable	&list_;
	size_type	pos_;
	bool		sorted_;

private:
	mutable size_type	tmp_pos		= 0;
	mutable const PairBlob	*tmp_blob	= nullptr;

	// We need to store the Pair,
	// because operator -> need somewhere Pair to live
	mutable ObserverPair	tmp_pair;
};

// ==============================

inline auto DiskTable::begin() const -> Iterator{
	return Iterator(*this,      0, sorted());
}

inline auto DiskTable::end() const -> Iterator{
	return Iterator(*this, size(), sorted());
}


} // namespace disk
} // namespace

#endif
