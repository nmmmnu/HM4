#ifndef POINTER_ITERATOR_H_
#define POINTER_ITERATOR_H_

#include <iterator>

template<class OIterator>
class pointer_iterator{
public:
	constexpr pointer_iterator() = default;

	explicit constexpr pointer_iterator(OIterator it) : ptr(it){}

public:
	using difference_type	= typename std::iterator_traits<OIterator>::difference_type;
	using value_type_ptr__	= typename std::iterator_traits<OIterator>::value_type;
//	using value_type	= std::remove_pointer_t<value_type_ptr__>;
//	using pointer		= value_type *;
//	using reference		= value_type &;
	using iterator_category	= std::random_access_iterator_tag;

public:
	// increment / decrement
	constexpr
	pointer_iterator &operator++(){
		++ptr;
		return *this;
	}

	constexpr
	pointer_iterator &operator--(){
		--ptr;
		return *this;
	}

	constexpr
	pointer_iterator operator++(int){
		auto tmp = ptr;
		++ptr;
		return { tmp };
	}

	constexpr
	pointer_iterator operator--(int){
		auto tmp = ptr;
		--ptr;
		return { tmp };
	}

public:
	// arithmetic
	// https://www.boost.org/doc/libs/1_50_0/boost/container/vector.hpp

	constexpr
	pointer_iterator& operator+=(difference_type const off){
		ptr += off;
		return *this;
	}

	constexpr pointer_iterator operator +(difference_type const off) const{
		return { ptr + off };
	}

	constexpr
	pointer_iterator& operator-=(difference_type const off){
		ptr -= off;
		return *this;
	}

	constexpr pointer_iterator operator -(difference_type const off) const{
		return pointer_iterator{ ptr - off };
	}

	friend constexpr pointer_iterator operator +(difference_type const  off, pointer_iterator const &it){
		return it.ptr + off;
	}

	constexpr difference_type operator -(pointer_iterator const &other) const{
		return ptr - other.ptr;
	}

public:
	// compare
	constexpr bool operator==(pointer_iterator const &other) const{
		return ptr == other.ptr;
	}

	constexpr bool operator!=(pointer_iterator const &other) const{
		return ptr != other.ptr;
	}

	constexpr bool operator >(pointer_iterator const &other) const{
		return ptr >  other.ptr;
	}

	constexpr bool operator>=(pointer_iterator const &other) const{
		return ptr >= other.ptr;
	}

	constexpr bool operator <(pointer_iterator const &other) const{
		return ptr <  other.ptr;
	}

	constexpr bool operator<=(pointer_iterator const &other) const{
		return ptr <= other.ptr;
	}

public:
	// dereference
	constexpr auto &operator*() const{
		return **ptr;
	}

	constexpr auto *operator->() const{
		return & operator*();
	}

	constexpr auto &operator[](difference_type const off) const{
		return *ptr[off];
	}

	constexpr auto *getPtr() const{
		return ptr;
	}

private:
	OIterator ptr{};
};

#endif

