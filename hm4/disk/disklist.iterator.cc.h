#include "logger.h"

namespace hm4{
namespace disk{



class DiskList::forward_iterator {
public:
	// begin or specific position
	forward_iterator(MMAPFilePlus const &mmap, const Pair *pair, bool const aligned) :
				mmap	(&mmap		),
				pair	(pair		),
				aligned	(aligned	){}

	// end
	forward_iterator() = default;

	using iterator = forward_iterator;

public:
	using difference_type = DiskList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	iterator &operator++(){
		if (pair)
			pair = fd_impl_::fdGetNext(*mmap, pair, aligned);

		return *this;
	}

	reference operator*() const{
		return *pair;
	}

public:
	bool operator==(iterator const &other) const{
		return pair == other.pair;
	}

	bool operator!=(iterator const &other) const{
		return ! operator==(other);
	}

	pointer operator ->() const{
		return & operator*();
	}

private:
	const MMAPFilePlus	*mmap	= nullptr;
	const Pair		*pair	= nullptr;
	bool			aligned	= false;
};



class DiskList::random_access_iterator{
public:
	random_access_iterator(DiskList const &list, difference_type const ptr) :
				list(&list),
				ptr(ptr){}

	random_access_iterator(DiskList const &list, size_type const ptr) :
				random_access_iterator(list, static_cast<difference_type>(ptr)){}

	explicit
	operator forward_iterator(){
		auto p = static_cast<size_type>(ptr);

		if (p >= list->size())
			return {};
		else
			return list->make_forward_iterator_(list->fdGetAt_(p));
	}

	using iterator = random_access_iterator;

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



} // namespace disk
} // namespace

