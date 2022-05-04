#ifndef MY_BUFFERED_VECTOR_H_
#define MY_BUFFERED_VECTOR_H_

#include <stdexcept>		// std::bad_alloc while push_back
#include <type_traits>		// std::is_trivial, std::is_standard_layout
#include <initializer_list>

//
// Based on
//	http://codereview.stackexchange.com/questions/123402/c-vector-the-basics
//	http://lokiastari.com/blog/2016/03/19/vector-simple-optimizations/
//

template<typename T, class Buffer>
class BufferedVector{
	static_assert(
		std::is_trivially_copyable		<T>::value	&&
		std::is_trivially_copy_constructible	<T>::value	&&
		std::is_trivially_move_constructible	<T>::value	&&
		std::is_trivially_destructible		<T>::value	&&
		std::is_standard_layout			<T>::value	&&

		true,	"T must be POD-like type"
	);

	static_assert(
		std::is_same_v<T, typename Buffer::value_type>,
		"T is different from Buffer::value_type"
	);
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

	Buffer		buffer_;

public:
	// STANDARD C-TORS

	template<typename... Args>
	constexpr BufferedVector(Args&&... args) : buffer_(std::forward<Args>(args)...){
	}

	template<typename... Args>
	constexpr BufferedVector(size_type const count, T const &value, Args&&... args) : BufferedVector(std::forward<Args>(args)...){
		assign(count, value);
	}

	template<class Iterator, typename... Args>
	constexpr BufferedVector(Iterator begin, Iterator end, Args&&... args) : BufferedVector(std::forward<Args>(args)...){
		assign(begin, end, std::forward<Args>(args)...);
	}

	template<typename... Args>
	constexpr BufferedVector(std::initializer_list<T> const &list, Args&&... args) :
		BufferedVector(list.begin(), list.end(), std::forward<Args>(args)...){}


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

	constexpr bool operator==(const BufferedVector &other) const noexcept{
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

	constexpr bool operator!=(const BufferedVector &other) const noexcept{
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
		return size_;
	}

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	// MORE Size

	constexpr size_type capacity() const noexcept{
		return buffer_.size();
	}

	constexpr size_type max_size() const noexcept{
		return buffer_.size();
	}

	// DATA

	constexpr
	value_type *data() noexcept{
		return buffer_.data();
	}

	constexpr const value_type *data() const noexcept{
		return buffer_.data();
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
		return data()[size_ - 1];
	}

	constexpr const value_type &back() const noexcept{
		// see [1] behavior is undefined
		return data()[size_ - 1];
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
		if (size_ == buffer_.size()){
			throw std::bad_alloc{};
		}

		data()[size_++] = value_type(std::forward<Args>(args)...);
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

	template<class Iterator>
	constexpr
	void assign(Iterator begin, Iterator end) {
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

