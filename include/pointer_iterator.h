#ifndef POINTER_ITERATOR_H_
#define POINTER_ITERATOR_H_

#include <iterator>

template<class OIterator>
class pointer_iterator{
public:
	constexpr pointer_iterator() = default;

	explicit constexpr pointer_iterator(OIterator it) : ptr(it){}

private:
	using T_ = typename std::iterator_traits<OIterator>;

private:
	using itt__		= typename std::iterator_traits<OIterator>;
	using value_type_ptr__	= typename itt__::value_type;

public:
	using iterator_category	= typename itt__::iterator_category;
	using difference_type	= typename itt__::difference_type;

	using value_type	= std::remove_pointer_t<value_type_ptr__>;
	using pointer		= value_type *;
	using reference		= value_type &;

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

namespace std {
	template <class OIterator>
	struct iterator_traits<pointer_iterator<OIterator> >{
		using T = pointer_iterator<OIterator>;

		using difference_type	= typename T::difference_type	;
		using iterator_category	= typename T::iterator_category	;
		using value_type	= typename T::value_type	;
		using pointer		= typename T::pointer		;
		using reference		= typename T::reference		;
	};
}

#endif

