#ifndef MY_FIXED_VECTOR_H_
#define MY_FIXED_VECTOR_H_

#include <stdexcept>		// std::bad_alloc while push_back
#include <type_traits>		// std::is_trivial, std::is_standard_layout
#include <initializer_list>

//
// Based on
//	http://codereview.stackexchange.com/questions/123402/c-vector-the-basics
//	http://lokiastari.com/blog/2016/03/19/vector-simple-optimizations/
//

template<typename T, std::size_t Size>
class FixedVector{
	static_assert(std::is_trivially_copyable<T>::value,	"T must be trivially copyable type");

public:
	// TYPES

	using value_type	= T;
	using size_type		= std::size_t;
	using difference_type	= std::ptrdiff_t;

	using reference		=       T&;
	using const_reference	= const T&;

	using pointer		=       T*;
	using const_pointer	= const T*;

	using iterator		=       T*;
	using const_iterator	= const T*;

private:
	size_type	size_	= 0;
	T		data_[Size]{};

public:
	// STANDARD C-TORS

	constexpr FixedVector() = default;

	constexpr FixedVector(size_type const count, T const &value) {
		assign(count, value);
	}

	template<class IT>
	constexpr FixedVector(IT begin, IT end){
		assign(begin, end);
	}

	constexpr FixedVector(std::initializer_list<T> const &list) :
		FixedVector(list.begin(), list.end()){}


	// MISC

	constexpr
	void reserve(size_type const) const noexcept{
		// left for compatibility
	}

	constexpr
	void clear() noexcept{
		size_ = 0;
	}

	// COMPARISSON

	constexpr bool operator==(const FixedVector &other) const noexcept{
		if (size_ != other.size_)
			return false;

		auto first = other.begin();
		auto last  = other.end();
		auto me    = begin();

		for(; first != last; ++first, ++me){
			if ( ! (*first == *me) )
				return false;
		}

		return true;
	}

	constexpr bool operator!=(const FixedVector &other) const noexcept{
		return ! operator==(other);
	}

	// ITERATORS

	constexpr
	iterator begin() noexcept{
		return data_;
	}

	constexpr
	iterator end() noexcept{
		return data_ + size_;
	}

	// CONST ITERATORS

	constexpr const_iterator begin() const noexcept{
		return data_;
	}

	constexpr const_iterator end() const noexcept{
		return data_ + size_;
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
		return size_;
	}

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	// MORE Size

	constexpr size_type capacity() const noexcept{
		return Size;
	}

	constexpr size_type max_size() const noexcept{
		return Size;
	}

	// DATA

	constexpr
	value_type *data() noexcept{
		return data_;
	}

	constexpr const value_type *data() const noexcept{
		return data_;
	}

	// ACCESS WITH RANGE CHECK

	constexpr
	value_type &at(size_type const index){
		validateIndex_(index);
		return data_[index];
	}

	constexpr const value_type &at(size_type const index) const{
		validateIndex_(index);
		return data_[index];
	}

	// ACCESS DIRECTLY

	constexpr
	value_type &operator[](size_type const index) noexcept{
		// see [1] behavior is undefined
		return data_[index];
	}

	constexpr const value_type &operator[](size_type const index) const noexcept{
		// see [1] behavior is undefined
		return data_[index];
	}

	// FRONT

	constexpr
	value_type &front() noexcept{
		// see [1] behavior is undefined
		return data_[0];
	}

	constexpr const value_type &front() const noexcept{
		// see [1] behavior is undefined
		return data_[0];
	}

	// BACK

	constexpr
	value_type &back() noexcept{
		// see [1] behavior is undefined
		return data_[size_ - 1];
	}

	constexpr const value_type &back() const noexcept{
		// see [1] behavior is undefined
		return data_[size_ - 1];
	}

	// MUTATIONS

	constexpr
	void push_back(const value_type &value){
		emplace_back(value);
	}

	constexpr
	void push_back(value_type &&value){
		emplace_back(std::move(value));
	}

	template<typename... Args>
	constexpr
	void emplace_back(Args&&... args){
		if (size_ == Size){
			throw std::bad_alloc{};
		}

		data_[size_++] = value_type(std::forward<Args>(args)...);
	}

	constexpr
	void pop_back() noexcept{
		// see [1]
		--size_;
	}

public:
	// APPEND

	constexpr
	void assign(size_type const count, T const &value) {
		for(size_type i = 0; i < count; ++i)
			push_back(value);
	}

	template<class IT>
	constexpr
	void assign(IT begin, IT end) {
		for(auto it = begin; it != end; ++it)
			push_back(*it);
	}

	constexpr
	void assign(std::initializer_list<T> const &list){
		assign(list.begin(), list.end());
	}

private:
	constexpr
	void validateIndex_(size_type const index) const{
		if (index >= size_){
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

