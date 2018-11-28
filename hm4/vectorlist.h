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
	class Iterator;

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

	const Pair *operator[](StringRef const &key) const;

public:
	Iterator lowerBound(StringRef const &key) const noexcept;
	Iterator begin() const noexcept;
	Iterator end() const noexcept;
};

// ==============================

class VectorList::Iterator{
private:
	friend class VectorList;
	constexpr Iterator(OVectorIt const &it) : ptr(it){}
	constexpr Iterator(OVectorIt &&it) : ptr(std::move(it)){}

public:
	constexpr Iterator(Iterator const &other) = default;
	constexpr Iterator(Iterator &&other) = default;

public:
	using difference_type = VectorList::difference_type;
	using value_type = const Pair;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::bidirectional_iterator_tag;

public:
	// increment / decrement
	Iterator &operator++(){
		++ptr;
		return *this;
	}

	Iterator &operator--(){
		--ptr;
		return *this;
	}

	Iterator operator++(int){
		auto tmp = ptr;
		++ptr;
		return { tmp };
	}

	Iterator operator--(int){
		auto tmp = ptr;
		--ptr;
		return { tmp };
	}

public:
	// arithmetic
	// https://www.boost.org/doc/libs/1_50_0/boost/container/vector.hpp

	Iterator& operator +=(difference_type const off){
		ptr += off;
		return *this;
	}

	Iterator operator +(difference_type const off) const{
		return { ptr + off };
	}

	Iterator& operator -=(difference_type const off){
		ptr -= off;
		return *this;
	}

	Iterator operator -(difference_type const off) const{
		return { ptr - off };
	}

	friend Iterator operator +(difference_type const  off, Iterator const &it){
		return it.ptr + off;
	}

	difference_type operator -(Iterator const &other) const{
		return ptr - other.ptr;
	}

public:
	// compare
	bool operator ==(Iterator const &other) const{
		return ptr == other.ptr;
	}

	bool operator !=(Iterator const &other) const{
		return ptr != other.ptr;
	}

	bool operator > (Iterator const &other) const{
		return ptr >  other.ptr;
	}

	bool operator >=(Iterator const &other) const{
		return ptr >= other.ptr;
	}

	bool operator < (Iterator const &other) const{
		return ptr <  other.ptr;
	}

	bool operator <=(Iterator const &other) const{
		return ptr <= other.ptr;
	}

public:
	// dereference
	reference operator *() const{
		return **ptr;
	}

	pointer operator ->() const{
		return & operator*();
	}

	reference operator [](difference_type const off) const{
		return *ptr[off];
	}

private:
	OVectorIt	ptr;
};

// ==============================

inline auto VectorList::begin() const noexcept -> Iterator{
	return std::begin(vector_);
}

inline auto VectorList::end() const noexcept -> Iterator{
	return std::end(vector_);
}

} // namespace


#endif
