#ifndef _VECTORLIST_H
#define _VECTORLIST_H

#include "ilist.h"
#include "listcounter.h"

#include "mynarrow.h"

#include <vector>

namespace hm4{

template<class T_Allocator>
class VectorList{
	using OVector	= std::vector<Pair *>;
	using OVectorIt	= OVector::const_iterator;

public:
	using Allocator		= T_Allocator;
	using size_type		= config::size_type;
	using difference_type	= config::difference_type;

public:
	class iterator;

public:
	VectorList(Allocator &allocator) : allocator_(& allocator){}

	VectorList(VectorList &&other) = default;
	~VectorList(){
		clear();
	}

private:
	OVector		vector_;
	ListCounter	lc_;
	Allocator	*allocator_;

public:
	bool clear();

	bool erase_(std::string_view key);

	Pair const &operator[](size_type const index) const{
		return *vector_[index];
	}

	template<class PFactory>
	iterator insertLazyPair_(PFactory &&factory);

	auto size() const{
		return lc_.size();
	}

	auto mutable_size() const{
		return size();
	}

	constexpr static void mutable_notify(const Pair *){
	}

	auto bytes() const{
		return lc_.bytes();
	}

	const Allocator &getAllocator() const{
		return *allocator_;
	}

	Allocator &getAllocator(){
		return *allocator_;
	}

public:
	iterator find(std::string_view key, std::true_type) const noexcept;
	iterator find(std::string_view key, std::false_type) const noexcept;
	iterator begin() const noexcept;
	iterator end() const noexcept;
};

// ==============================

template<class T_Allocator>
class VectorList<T_Allocator>::iterator{
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
		return { tmp };
	}

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

	iterator operator +(difference_type const off) const{
		return { ptr + off };
	}

	iterator& operator-=(difference_type const off){
		ptr -= off;
		return *this;
	}

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

template<class T_Allocator>
inline auto VectorList<T_Allocator>::begin() const noexcept -> iterator{
	return std::begin(vector_);
}

template<class T_Allocator>
inline auto VectorList<T_Allocator>::end() const noexcept -> iterator{
	return std::end(vector_);
}



} // namespace



#endif
