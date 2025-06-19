#ifndef MY_STATIC_VECTOR_H_
#define MY_STATIC_VECTOR_H_

#include <memory>		// std::uninitialized_copy, std::uninitialized_move
#include <stdexcept>		// std::bad_alloc
#include <type_traits>
#include <initializer_list>

//
// Based on
//	http://codereview.stackexchange.com/questions/123402/c-vector-the-basics
//	http://lokiastari.com/blog/2016/03/19/vector-simple-optimizations/
//

namespace static_vector_implementation_{
	template <typename Derived, typename T, std::size_t Capacity, bool Trivial>
	struct StaticVectorBase_;



	template <typename Derived, typename T, std::size_t Capacity>
	struct StaticVectorBase_<Derived, T, Capacity, true>{
		constexpr static bool IS_POD = true;

		using value_type	= T;
		using size_type		= std::size_t;

	public:
		constexpr value_type *data() noexcept{
			return buffer_;
		}

		constexpr const value_type *data() const noexcept{
			return buffer_;
		}

		constexpr static size_type capacity() noexcept{
			return Capacity;
		}

	protected:
		value_type	buffer_[Capacity]{};
		size_type	size_ = 0;
	};



	template <typename Derived, typename T, std::size_t Capacity>
	struct StaticVectorBase_<Derived, T, Capacity, false>{
		constexpr static bool IS_POD = false;

		using value_type	= T;
		using size_type		= std::size_t;

	public:
		constexpr value_type *data() noexcept{
			return reinterpret_cast<value_type *>(buffer_);
		}

		constexpr const value_type *data() const noexcept{
			return reinterpret_cast<const value_type *>(buffer_);
		}

		constexpr static size_type capacity() noexcept{
			return Capacity;
		}

	public:
		~StaticVectorBase_() noexcept {
			Derived &self = static_cast<Derived &>(*this);

			self.deallocate_();
		}

	protected:
		alignas(value_type)
		char		buffer_[Capacity * sizeof(value_type)]{};
		size_type	size_ = 0;
	};



	template <typename Derived, typename T>
	struct StaticVectorBase_<Derived, T, 0, true>{
		constexpr static bool IS_POD = true;

		using value_type	= T;
		using size_type		= std::size_t;

	public:
		StaticVectorBase_(value_type *buffer, size_type capacity) : buffer_(buffer), capacity_(capacity){}

	public:
		constexpr value_type *data() noexcept{
			return buffer_;
		}

		constexpr const value_type *data() const noexcept{
			return buffer_;
		}

		constexpr size_type capacity() const noexcept{
			return capacity_;
		}

	protected:
		value_type	*buffer_;
		size_type	capacity_;
		size_type	size_ = 0;
	};



	template <typename Derived, typename T>
	struct StaticVectorBase_<Derived, T, 0, false>{
		constexpr static bool IS_POD = false;

		using value_type	= T;
		using size_type		= std::size_t;

	public:
		StaticVectorBase_(char *buffer, size_type capacity) : buffer_(buffer), capacity_(capacity){}

	public:
		constexpr value_type *data() noexcept{
			return reinterpret_cast<value_type *>(buffer_);
		}

		constexpr const value_type *data() const noexcept{
			return reinterpret_cast<const value_type *>(buffer_);
		}

		constexpr size_type capacity() const noexcept{
			return capacity_;
		}

	public:
		// class is not movable

		~StaticVectorBase_() noexcept {
			Derived &self = static_cast<Derived &>(*this);

			self.deallocate_();
		}

	protected:
		char		*buffer_;
		size_type	capacity_;
		size_type	size_ = 0;
	};

} // namespace static_vector_implementation_



template<typename T, std::size_t Capacity>
class StaticVector : public static_vector_implementation_::StaticVectorBase_<
						StaticVector<T, Capacity>,
						T,
						Capacity,
						std::is_trivial_v<T> && std::is_standard_layout_v<T>
				>{

	using Base = static_vector_implementation_::StaticVectorBase_<
						StaticVector<T, Capacity>,
						T,
						Capacity,
						std::is_trivial_v<T> && std::is_standard_layout_v<T>
				>;

	using Base::IS_POD;

	template <typename, typename, std::size_t, bool>
	friend struct static_vector_implementation_::StaticVectorBase_;

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

private:
	// size_type	size_	= 0;

	using Base::size_;

public:
	// STANDARD C-TORS FOR Capacity != 0

	constexpr StaticVector() = default;

	constexpr StaticVector(size_type const count, value_type const &value){
		assign_(count, value);
	}

	template<class Iterator>
	constexpr StaticVector(Iterator begin, Iterator end){
		assign_(begin, end);
	}

	template<typename ...Args>
	constexpr StaticVector(std::initializer_list<value_type> const &list){
		assign_(list.begin(), list.end());
	}

	// STANDARD C-TORS FOR Capacity == 0

	template<typename buffer_value_type>
	constexpr StaticVector(
				buffer_value_type *data, size_type size) : Base(data, size){
	}

	template<typename buffer_value_type>
	constexpr StaticVector(size_type const count, value_type const &value,
				buffer_value_type *data, size_type size) : Base(data, size){
		assign_(count, value);
	}

	template<class Iterator, typename buffer_value_type>
	constexpr StaticVector(Iterator begin, Iterator end,
				buffer_value_type *data, size_type size) : Base(data, size){
		assign_(begin, end);
	}

	template<typename buffer_value_type>
	constexpr StaticVector(std::initializer_list<value_type> const &list,
				buffer_value_type *data, size_type size) : Base(data, size){
		assign_(list.begin(), list.end());
	}

	// COPY / MOVE C-TORS

	constexpr StaticVector(StaticVector const &other){
		static_assert(Capacity, "BufferedVector can not be copied/moved!");

		copy_<0>(other.begin(), other.end());
	}

	constexpr StaticVector(StaticVector &&other) noexcept{
		static_assert(Capacity, "BufferedVector can not be copied/moved!");

		move_<0>(other.begin(), other.end());
	}

	constexpr StaticVector &operator=(StaticVector const &other) {
		static_assert(Capacity, "BufferedVector can not be copied/moved!");

		if (this == &other)
			return *this;

		copy_<1>(other.begin(), other.end());

		return *this;
	}

	constexpr StaticVector &operator=(StaticVector &&other) noexcept {
		static_assert(Capacity, "BufferedVector can not be copied/moved!");

		if (this == &other)
			return *this;

		move_<1>(other.begin(), other.end());

		return *this;
	}

	// MISC

	constexpr
	void reserve(size_type const) noexcept{
	}

	constexpr
	void clear() noexcept{
		destroy_();

		size_ = 0;
	}

	// COMPARISSON

	constexpr bool operator==(const StaticVector &other) const noexcept{
		if (size_ != other.size_)
			return false;

		// std::equal, but constexpr

		auto first = other.begin();
		auto last  = other.end();
		auto me    = begin();

		for(; first != last; ++first, ++me)
			if ( ! (*first == *me) )
				return false;

		return true;
	}

	constexpr bool operator!=(const StaticVector &other) const noexcept{
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

	constexpr bool full() const noexcept{
		return size() == capacity();
	}

	// MORE Size

	// constexpr size_type capacity() const noexcept;

	using Base::capacity;

	constexpr size_type max_size() const noexcept{
		return capacity();
	}

	// DATA - in Base

	// constexpr       value_type *data()       noexcept;
	// constexpr const value_type *data() const noexcept;

	using Base::data;

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
		if (full())
			throw std::bad_alloc{};

		if constexpr(!IS_POD){
			new (&data()[size_]) value_type(std::forward<Args>(args)...);

			// incr is here because new can throw exception,
			// in some theoretical case

			++size_;
		}else{
			data()[size_++] = value_type(std::forward<Args>(args)...);
		}
	}

	// POP_BACK

	constexpr
	void pop_back() noexcept{
		// see [1]

		if constexpr(!IS_POD)
			back().~value_type();

		--size_;
	}

public:
	// APPEND

	constexpr
	void assign(size_type const count, value_type const &value){
		assign_<1>(count, value);
	}

	template<class Iterator>
	constexpr
	void assign(Iterator first, Iterator last){
		assign_<1>(first, last);
	}

	constexpr
	void assign(std::initializer_list<value_type> const &list){
		assign(list.begin(), list.end());
	}

private:
	// construct / copy elements from vector in exception safe way

	template<bool Destruct>
	constexpr
	void assign_(size_type const count, value_type const &value){
		if constexpr(Destruct)
			clear();

		// reserve(count);

		for(size_type i = 0; i < count; ++i)
			push_back(value);
	}

	template<bool Destruct, class Iterator>
	constexpr
	void assign_(Iterator first, Iterator last){
		if constexpr(Destruct)
			clear();

		// reserve(std::distance(first, last));

		for(auto it = first; it != last; ++it)
			push_back(*it);
	}

	// copy / move elements

	template<bool Destruct, class Iterator>
	constexpr
	void copy_(Iterator first, Iterator last){
		if constexpr(std::is_nothrow_copy_constructible_v<value_type>){

			#if defined(__clang__) || defined(__GNUC__)

			if (__builtin_is_constant_evaluated()){
				// constant evaluation,
				// assign is constexpr and exception friendly
				return assign_<Destruct>(first, last);
			}

			# endif

			if constexpr(Destruct)
				destroy_();

			size_type const size = static_cast<size_type>(std::distance(first, last));
			// reserve(size);
			size_ = size;

			std::uninitialized_copy(first, last, data());
		}else{
			assign_<Destruct>(first, last);
		}
	}

	template<bool Destruct, class Iterator>
	constexpr
	void move_(Iterator first, Iterator last){
		if constexpr(IS_POD){
			// POD move is copy
			return copy_<Destruct>(first, last);
		}

		if constexpr(std::is_nothrow_move_constructible_v<value_type>){

			#if defined(__clang__) || defined(__GNUC__)

			if (__builtin_is_constant_evaluated()){
				// constant evaluation,
				// assign is constexpr and exception friendly
				return assign_<Destruct>(first, last);
			}

			# endif

			if constexpr(Destruct)
				destroy_();

			size_type const size = static_cast<size_type>(std::distance(first, last));
			// reserve(size);
			size_ = size;

			std::uninitialized_move(first, last, data());
		}else{
			assign_<Destruct>(first, last);
		}
	}

	// destruct all elements

	constexpr
	void destroy_() noexcept{
		if constexpr(!IS_POD)
			std::destroy(begin(), end());
	}

	// destruct all elements and "deallocate" the buffer

	constexpr
	void deallocate_() noexcept{
		destroy_();
	}

	// throw if index is incorrect

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

template<typename T>
using BufferedVector = StaticVector<T, 0>;

#endif

