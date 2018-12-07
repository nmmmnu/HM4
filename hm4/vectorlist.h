#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"

#include "mynarrow.h"
#include "binarysearch.h"

#include <cassert>
#include <vector>

namespace hm4{


class VectorList{
	using OVector	= std::vector<OPair>;
	using OVectorIt	= OVector::const_iterator;

public:
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	VectorList() = default;

private:
	OVector		vector_;
	size_t		dataSize_ = 0;

public:
	bool clear(){
		vector_.clear();
		dataSize_ = 0;
		return true;
	}

	bool erase(const StringRef &key);

	const Pair *operator[](size_type const index) const{
		assert( index < size() );

		return vector_[index].get();
	}

	bool insert(OPair &&data);

	size_type size() const{
		return vector_.size();
	}

	size_type sizeEstimated() const{
		return size();
	}

	size_t bytes() const{
		return dataSize_;
	}

public:
	iterator find(StringRef const &key, bool exact) const noexcept;
	iterator begin() const noexcept;
	iterator end() const noexcept;
};

// ==============================

class VectorList::iterator{
public:
	constexpr iterator(OVectorIt const &it) : ptr(it){}
	constexpr iterator(OVectorIt &&it) : ptr(std::move(it)){}

public:
	using difference_type	= VectorList::difference_type;
	using value_type	= const Pair;
	using pointer		= value_type *;
	using reference		= value_type &;
	using iterator_category	= std::random_access_iterator_tag;

public:
	// increment / decrement
	constexpr
	iterator &operator++(){
		++ptr;
		return *this;
	}

	constexpr
	iterator &operator--(){
		--ptr;
		return *this;
	}

	constexpr
	iterator operator++(int){
		auto tmp = ptr;
		++ptr;
		return { tmp };
	}

	constexpr
	iterator operator--(int){
		auto tmp = ptr;
		--ptr;
		return { tmp };
	}

public:
	// arithmetic
	// https://www.boost.org/doc/libs/1_50_0/boost/container/vector.hpp

	iterator& operator+=(difference_type const off){
		ptr += off;
		return *this;
	}

	constexpr
	iterator operator +(difference_type const off) const{
		return { ptr + off };
	}

	iterator& operator-=(difference_type const off){
		ptr -= off;
		return *this;
	}

	constexpr
	iterator operator -(difference_type const off) const{
		return { ptr - off };
	}

	friend iterator operator +(difference_type const  off, iterator const &it){
		return it.ptr + off;
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
	reference operator*() const{
		return **ptr;
	}

	pointer operator->() const{
		return & operator*();
	}

	reference operator[](difference_type const off) const{
		return *ptr[off];
	}

private:
	OVectorIt	ptr;
};

// ==============================

inline auto VectorList::begin() const noexcept -> iterator{
	return std::begin(vector_);
}

inline auto VectorList::end() const noexcept -> iterator{
	return std::end(vector_);
}

} // namespace


#endif
