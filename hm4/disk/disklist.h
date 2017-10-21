#ifndef DISK_LIST_H_
#define DISK_LIST_H_

#include "mmapfileplus.h"

#include "ilist.h"
#include "filemeta.h"

namespace hm4{
namespace disk{


class DiskList : public IList<DiskList, false>{
public:
	class Iterator;
	class BTreeSearchHelper;

	static constexpr MMAPFile::Advice DEFAULT_ADVICE = MMAPFile::Advice::RANDOM;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;
	static constexpr int		CMP_ZERO			= 1;

public:
	DiskList() = default;

	DiskList(DiskList &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	// ~DiskList() = default;

	bool open(const StringRef &filename, MMAPFile::Advice advice = DEFAULT_ADVICE);
	void close();

	operator bool(){
		return metadata_;
	}

	void printMetadata() const{
		metadata_.print();
	}

public:
	const Pair *operator[](const StringRef &key) const{
		assert(!key.empty());

		const auto x = search_(key);

		return x.first ? operator[](x.second) : nullptr;
	}

	const Pair *operator[](size_type const index) const{
		assert( index < size() );

		return getAtFD_(index);
	}

	int cmpAt(size_type index, const StringRef &key) const{
		assert(!key.empty());

		const Pair *p = operator[](index);

		// StringRef is not null terminated
		return p ? p->cmp(key) : CMP_ZERO;
	}

	size_type size(bool const = false) const{
		return (size_type) metadata_.size();
	}

	size_t bytes() const{
		return mData_.size();
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
	const Pair *saveAccessFD_(const Pair *blob) const;

	const Pair *getAtFD_(size_type index) const;
	const Pair *getNextFD_(const Pair *blob) const;

	static size_t getSizeFD__(const Pair *blob, bool aligned);

private:
	std::pair<bool,size_type> binarySearch_(const StringRef &key) const;
	std::pair<bool,size_type> btreeSearch_(const StringRef &key) const;

	std::pair<bool,size_type> search_(const StringRef &key) const;

private:
	MMAPFilePlus		mIndx_;
	MMAPFilePlus		mData_;

	MMAPFilePlus		mTree_;
	MMAPFilePlus		mKeys_;

	FileMeta		metadata_;
};

// ===================================

class DiskList::Iterator{
private:
	friend class DiskList;
	Iterator(const DiskList &list, size_type pos,
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

	const Pair &operator*() const;

public:
	bool operator!=(const Iterator &other) const{
		return ! operator==(other);
	}

	const Pair *operator ->() const{
		return & operator*();
	}

private:
	const DiskList	&list_;
	size_type	pos_;
	bool		sorted_;

private:
	mutable
	size_type	tmp_pos		= 0;

	mutable
	const Pair	*tmp_blob	= nullptr;
};

// ==============================

inline auto DiskList::begin() const -> Iterator{
	return Iterator(*this,      0, sorted());
}

inline auto DiskList::end() const -> Iterator{
	return Iterator(*this, size(), sorted());
}

inline auto DiskList::lowerBound(const StringRef &key) const -> Iterator{
	const auto x = search_(key);

	return Iterator(*this, x.second, sorted());
}


} // namespace disk
} // namespace

#endif

