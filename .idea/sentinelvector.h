#ifndef MY_SENTINEL_VECTOR_H_
#define MY_SENTINEL_VECTOR_H_

#include <stdexcept>		// std::bad_alloc
#include <type_traits>
#include <initializer_list>

//
// Based on
//	http://codereview.stackexchange.com/questions/123402/c-vector-the-basics
//	http://lokiastari.com/blog/2016/03/19/vector-simple-optimizations/
//

template<typename T, T Sentinel, std::size_t Capacity>
class SentinelVector{
	static_assert(std::is_trivial_v<T> && std::is_standard_layout_v<T>, "T must be POD");

	T data_[Capacity];

public:
	// TYPES

	using value_type	= T;
	using size_type		= std::size_t;
	using difference_type	= std::ptrdiff_t;

	using reference		=       value_type &;
	using const_reference	= const value_type &;

	using pointer		=       value_type *;
	using const_pointer	= const value_type *;

	using iterator		=       value_type *;
	using const_iterator	= const value_type *;

public:
	// STANDARD C-TORS FOR Capacity != 0

	constexpr SentinelVector(){
		clear();
	}

	constexpr SentinelVector(size_type const count, value_type const &value){
		construct_(count, value);
	}

	template<class Iterator>
	constexpr SentinelVector(Iterator begin, Iterator end){
		copy_(begin, end);
	}

	template<typename ...Args>
	constexpr SentinelVector(std::initializer_list<value_type> const &list){
		copy_(list.begin(), list.end());
	}

	// MISC

	constexpr
	void reserve(size_type const) noexcept{
	}

	constexpr
	void clear() noexcept{
		data()[0] = Sentinel;
	}

	// COMPARISSON

	constexpr bool operator==(SentinelVector const &other) const noexcept{
		// std::equal, but constexpr

		auto me    = data_.begin();
		auto end   = data_.end();

		auto ot    = other.data_.begin();

		for(; me != end; ++ot, ++me){
			if (*me != *ot)
				return false;

			// *me == *ot

			if (*me == Sentinel)
				return true;
		}

		return true;
	}

	constexpr bool operator!=(SentinelVector const &other) const noexcept{
		return ! operator==(other);
	}

	// ITERATORS

	constexpr
	iterator begin() noexcept{
		return data();
	}

	constexpr
	iterator end() noexcept{
		return data() + size();
	}

	// CONST ITERATORS

	constexpr const_iterator begin() const noexcept{
		return data();
	}

	constexpr const_iterator end() const noexcept{
		return data() + size();
	}

	// C++11 CONST ITERATORS

	constexpr const_iterator cbegin() const noexcept{
		return begin();
	}

	constexpr const_iterator cend() const noexcept{
		return end();
	}

	// Size

	constexpr size_type size() const noexcept{
		size_type size = 0;

		for(auto &x : data_){
			if (x == Sentinel)
				return size;

			++size;
		}

		return size;
	}

	constexpr bool empty() const noexcept{
		return data()[0] == Sentinel;
	}

	constexpr bool full() const noexcept{
		return size() == capacity();
	}

	// MORE Size

	constexpr size_type capacity() const noexcept{
		return Capacity;
	}

	constexpr size_type max_size() const noexcept{
		return capacity();
	}

	// DATA

	constexpr value_type *data() noexcept{
		return data_;
	}

	constexpr const value_type *data() const noexcept{
		return data_;
	}

	// ACCESS WITH RANGE CHECK

	constexpr
	value_type &at(size_type const index){
		validateIndex_(index);
		return data()[index];
	}

	constexpr const value_type &at(size_type const index) const{
		validateIndex_(index);
		return data()[index];
	}

	// ACCESS DIRECTLY

	constexpr
	value_type &operator[](size_type const index) noexcept{
		// see [1] behavior is undefined
		return data()[index];
	}

	constexpr const value_type &operator[](size_type const index) const noexcept{
		// see [1] behavior is undefined
		return data()[index];
	}

	// FRONT

	constexpr
	value_type &front() noexcept{
		// see [1] behavior is undefined
		return data()[0];
	}

	constexpr const value_type &front() const noexcept{
		// see [1] behavior is undefined
		return data()[0];
	}

	// BACK

	constexpr
	value_type &back() noexcept{
		// see [1] behavior is undefined
		return data()[size() - 1];
	}

	constexpr const value_type &back() const noexcept{
		// see [1] behavior is undefined
		return data()[size() - 1];
	}

	// MUTATIONS

	constexpr
	void push_back(){
		return emplace_back();
	}

	constexpr
	void push_back(const value_type &value){
		return emplace_back(value);
	}

	constexpr
	void push_back(value_type &&value){
		return emplace_back(std::move(value));
	}

	template<typename... Args>
	constexpr
	void emplace_back(Args&&... args){
		auto const index = size();

		if (index == capacity())
			throw std::bad_alloc{};

		data()[index] = value_type(std::forward<Args>(args)...);

		if (index + 1 < capacity())
			data()[index + 1] = Sentinel;
	}

	// POP_BACK

	constexpr
	void pop_back() noexcept{
		// see [1]

		back() = Sentinel;
	}

public:
	// APPEND

	constexpr
	void assign(size_type const count, value_type const &value){
		construct_(count, value);
	}

	template<class Iterator>
	constexpr
	void assign(Iterator first, Iterator last){
		copy_(first, last);
	}

	constexpr
	void assign(std::initializer_list<value_type> const &list){
		copy_(list.begin(), list.end());
	}

private:
	// construct / copy elements from vector in exception safe way

	constexpr
	void construct_(size_type const count, value_type const &value){
		if (count > capacity())
			throw std::bad_alloc{};

		clear();

		// reserve(count);

		for (size_type i = 0; i < count; ++i)
			data_[i] = value;

		if (count < capacity())
			data_[count] = Sentinel;
	}

	// copy / move elements

	template<class Iterator>
	constexpr
	void copy_(Iterator first, Iterator last){
		size_type const count = std::distance(first, last);

		if (count > capacity())
			throw std::bad_alloc{};

		clear();

		// reserve(count);

		for (size_type i = 0; i < count; ++i, ++first)
			data_[i] = *first;

		if (count < capacity())
			data_[count] = Sentinel;
	}

	// throw if index is incorrect

	constexpr
	void validateIndex_(size_type const index) const{
		if (index >= size()){
			throw std::out_of_range("Out of Range");
		}
	}

	// Remark [1]
	//
	// If the container is not empty,
	// the function never throws exceptions (no-throw guarantee).
	// Otherwise, it causes undefined behavior.

};

#endif

#include <cstdio>

int main(){
	SentinelVector<int, -1, 8> v{ 100, 200 };

	v.push_back(1);
	v.push_back(2);
	v.push_back(3);
	v.push_back(4);

	printf("%d\n", v.front());
	printf("%d\n", v.back());

	for(auto const &x : v)
		printf("%d\n", x);
}

