#ifndef DISK_LIST_H_
#define DISK_LIST_H_

#include "mmapfileplus.h"

#include "ilist.h"
#include "filemeta.h"

#include <type_traits>

namespace hm4{
namespace disk{


class DiskList{
public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

	class iterator;

public:
	enum class OpenMode : char {
		NORMAL,
		MINIMAL
	};

	static constexpr MMAPFile::Advice	DEFAULT_ADVICE	= MMAPFile::Advice::RANDOM;
	static constexpr OpenMode		DEFAULT_MODE	= OpenMode::NORMAL;

private:
	static constexpr size_type	BIN_SEARCH_MINIMUM_DISTANCE	= 3;
//	static constexpr int		CMP_ZERO			= 1;

public:
	DiskList() = default;

	DiskList(DiskList &&other) = default;

	// no need d-tor,
	// MMAPFile-s will be closed automatically
	// ~DiskList() = default;

	void close();

	bool open(const StringRef &filename, MMAPFile::Advice const advice = DEFAULT_ADVICE, OpenMode const mode = DEFAULT_MODE){
		switch(mode){
		default:
		case OpenMode::NORMAL	: return openNormal_ (filename, advice);
		case OpenMode::MINIMAL	: return openMinimal_(filename, advice);
		}
	}

	operator bool(){
		return metadata_;
	}

	void printMetadata() const{
		metadata_.print();
	}

public:
	Pair const &operator[](size_type const index) const{
		assert( index < size() );

		return *fdGetAt_(index);
	}

	size_type size() const{
		return metadata_.size();
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
	iterator begin() const;
	iterator end() const;

	template<bool B>
	iterator find(const StringRef &key, std::bool_constant<B>) const;

private:
	template<bool B>
	iterator search_(const StringRef &key, std::bool_constant<B>) const;

	bool openNormal_ (const StringRef &filename, MMAPFile::Advice advice);
	bool openMinimal_(const StringRef &filename, MMAPFile::Advice advice);

private:
	const Pair *fdSafeAccess_(const Pair *blob) const;

	const Pair *fdGetAt_(size_type index) const;
#if 0
	const Pair *fdGetNext_(const Pair *blob) const;
	static size_t alignedSize__(const Pair *blob, bool aligned);
#endif

public:
	class BTreeSearchHelper;

private:
	MMAPFilePlus		mIndx_;
	MMAPFilePlus		mLine_;
	MMAPFilePlus		mData_;

	MMAPFilePlus		mTree_;
	MMAPFilePlus		mKeys_;

	FileMeta		metadata_;
};

// ===================================

class DiskList::iterator{
public:
	using difference_type	= DiskList::difference_type;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::random_access_iterator_tag;

private:
	using size_type		= DiskList::size_type;

private:
	iterator clone(difference_type const off) const{
		return { *list, off };
	}

	reference getAt(difference_type const off) const{
		return (*list)[ static_cast<size_type>(off) ];
	}

public:
	iterator(DiskList const &list, difference_type const ptr) :
				list(&list),
				ptr(ptr){}

	iterator(DiskList const &list, size_type const ptr) :
				iterator(list, static_cast<difference_type>(ptr)){}

public:
	// increment / decrement
	iterator &operator++(){
		++ptr;
		return *this;
	}

	iterator &operator--(){
		--ptr;
		return *this;
	}

	iterator operator++(int){
		auto tmp = ptr;
		++ptr;
		return clone(tmp);
	}

	iterator operator--(int){
		auto tmp = ptr;
		--ptr;
		return clone(tmp);
	}

public:
	// arithmetic
	// https://www.boost.org/doc/libs/1_50_0/boost/container/vector.hpp

	iterator& operator+=(difference_type const off){
		ptr += off;
		return *this;
	}

	iterator operator +(difference_type const off) const{
		return clone(ptr + off);
	}

	iterator& operator-=(difference_type const off){
		ptr -= off;
		return *this;
	}

	iterator operator -(difference_type const off) const{
		return clone(ptr - off);
	}

	friend iterator operator +(difference_type const  off, iterator const &it){
		return it.clone(it.ptr + off);
	}

	difference_type operator -(iterator const &other) const{
		return ptr - other.ptr;
	}

public:
	// compare
	bool operator==(iterator const &other) const{
		return ptr == other.ptr;
	}

	bool operator!=(iterator const &other) const{
		return ptr != other.ptr;
	}

	bool operator >(iterator const &other) const{
		return ptr >  other.ptr;
	}

	bool operator>=(iterator const &other) const{
		return ptr >= other.ptr;
	}

	bool operator <(iterator const &other) const{
		return ptr <  other.ptr;
	}

	bool operator<=(iterator const &other) const{
		return ptr <= other.ptr;
	}

public:
	// dereference

	reference operator[](difference_type const off) const{
		return getAt(ptr + off);
	}

	reference operator*() const{
		return getAt(ptr);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const DiskList	*list;
	difference_type	ptr;
};

// ==============================

inline auto DiskList::begin() const -> iterator{
	return { *this, difference_type{ 0 } };
}

inline auto DiskList::end() const -> iterator{
	return { *this, size() };
}

template<bool B>
inline auto DiskList::find(const StringRef &key, std::bool_constant<B> const exact) const -> iterator{
	return search_(key, exact);
}


} // namespace disk
} // namespace

#endif

