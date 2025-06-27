#ifndef MY_SMALL_VECTOR_H_
#define MY_SMALL_VECTOR_H_

#include <memory>		// std::uninitialized_copy, std::uninitialized_move
#include <limits>
#include <initializer_list>

#include <cassert>

template<typename T, std::size_t BufferSize>
struct SmallVector{
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
	constexpr static size_type GrowFactor = 2;

private:
	size_type	size_		= 0;
	size_type	capacity_	= BufferSize;
	value_type	*data_		= reinterpret_cast<value_type *>(buffer_);
	alignas(value_type)
	char		buffer_[BufferSize * sizeof(value_type)];

public:
	// STANDARD C-TORS

	constexpr SmallVector() = default;

	constexpr SmallVector(size_type const count, value_type const &value){
		assign_<0>(count, value);
	}

	template<class Iterator>
	constexpr SmallVector(Iterator begin, Iterator end){
		assign_<0>(begin, end);
	}

	constexpr SmallVector(std::initializer_list<value_type> const &list){
		assign_<0>(list.begin(), list.end());
	}

	// D-TOR

	~SmallVector() noexcept {
		deallocate_();
	}

	// COPY / MOVE C-TORS

	constexpr SmallVector(SmallVector const &other){
		copy_<0>(other.begin(), other.end());
	}

	constexpr SmallVector(SmallVector &&other) noexcept{
		if (other.useInternalBuffer_()){
			move_<0>(other.begin(), other.end());
		}else{
			// plunder other's external buffer

			data_     = other.data_;
			size_     = other.size_;
			capacity_ = other.capacity_;

			other.reset_();
		}
	}

	constexpr SmallVector &operator=(SmallVector const &other) {
		if (this == &other)
			return *this;

		copy_<1>(other.begin(), other.end());

		return *this;
	}

	constexpr SmallVector &operator=(SmallVector &&other) noexcept {
		if (this == &other)
			return *this;

		if (other.useInternalBuffer_()){
			move_<1>(other.begin(), other.end());

		}else if (!useInternalBuffer_()){
			// both use external buffer
			// swap external buffers

			using std::swap;

			swap(data_	, other.data_		 );
			swap(size_	, other.size_		 );
			swap(capacity_	, other.capacity_	 );
		}else{
			deallocate_();

			// plunder other's external buffer

			data_     = other.data_;
			size_     = other.size_;
			capacity_ = other.capacity_;

			other.reset_();
		}

		return *this;
	}

	// SWAP

	// constexpr
	// void swap(SmallVector &other){
	// 	using std::swap;
	//
	// 	swap(data_	, other.data_		 );
	// 	swap(size_	, other.size_		 );
	// 	swap(capacity_	, other.capacity_	 );
	// }
	//
	// constexpr
	// friend void swap(SmallVector &a, SmallVector &b){
	// 	a.swap(b);
	// }

	// MISC

	constexpr
	void reserve(size_type const capacity){
		if (capacity > capacity_)
			reallocate_(capacity);
	}

	constexpr
	void clear() noexcept{
		destroy_();

		size_ = 0;
	}

	// COMPARISSON

	constexpr bool operator==(const SmallVector &other) const noexcept{
		if (size_ != other.size_)
			return false;

		return size_ == other.size_ && std::equal(begin(), end(), other.begin());
	}

	constexpr bool operator!=(const SmallVector &other) const noexcept{
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
		return capacity_;
	}

	constexpr size_type max_size() const noexcept{
		return std::numeric_limits<size_type>::max();
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
		if (size_ == capacity_)
			reallocate_(capacity_ * GrowFactor);

		new (&data()[size_]) value_type(std::forward<Args>(args)...);

		// incr is here because new can throw exception,
		// in some theoretical case

		++size_;
	}

	// POP_BACK

	constexpr
	void pop_back() noexcept{
		// see [1]

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
	// helper for check if internal buffer is in use

	constexpr bool useInternalBuffer_(const value_type *data) const{
		const void *p1 = data;
		const void *p2 = buffer_;

		return p1 == p2;
	}

	constexpr bool useInternalBuffer_() const{
		return useInternalBuffer_(data_);
	}

	// reset vector to use internal buffer

	constexpr void reset_(){
		size_		= 0;
		capacity_	= BufferSize;
		data_		= reinterpret_cast<value_type *>(buffer_);
	}

	// construct / copy elements from vector in exception safe way

	template<bool Destruct>
	constexpr
	void assign_(size_type const count, value_type const &value){
		if constexpr(Destruct)
			clear();

		reserve(count);

		for(size_type i = 0; i < count; ++i)
			push_back(value);
	}

	template<bool Destruct, class Iterator>
	constexpr
	void assign_(Iterator first, Iterator last){
		if constexpr(Destruct)
			clear();

		reserve(static_cast<size_type>(std::distance(first, last)));

		for(auto it = first; it != last; ++it)
			push_back(*it);
	}

	// copy / move elements

	template<bool Destruct, class Iterator>
	constexpr
	void copy_(Iterator first, Iterator last){
		if constexpr(std::is_nothrow_copy_constructible_v<value_type>){
			if constexpr(Destruct)
				destroy_();

			size_type const size = static_cast<size_type>(std::distance(first, last));
			reserve(size);
			size_ = size;

			std::uninitialized_copy(first, last, data());
		}else{
			assign_<Destruct>(first, last);
		}
	}

	template<bool Destruct, class Iterator>
	constexpr
	void move_(Iterator first, Iterator last){
		if constexpr(std::is_nothrow_move_constructible_v<value_type>){
			if constexpr(Destruct)
				destroy_();

			size_type const size = static_cast<size_type>(std::distance(first, last));
			reserve(size);
			size_ = size;

			std::uninitialized_move(first, last, data());
		}else{
			assign_<Destruct>(first, last);
		}
	}

	// destruct all elements

	constexpr
	void destroy_() noexcept{
		std::destroy(begin(), end());
	}

	// destruct all elements and deallocate the buffer

	constexpr
	void deallocate_(value_type *data, size_type size) noexcept{
		std::destroy(data, data + size);

		if (!useInternalBuffer_(data))
			::operator delete(data);
	}

	constexpr
	void deallocate_(){
		deallocate_(data_, size_);
	}

	// reallocate

	constexpr
	void reallocate_(size_type const capacity){
		// printf("reallocate %zu, %zu, [%p], %p\n", capacity_, capacity, (void *) buffer_, (void *) data_);

		assert(capacity > capacity_);

		void *p        = ::operator new(capacity * sizeof(value_type));
		auto *old_data = data_;
		data_          = static_cast<value_type *>(p);

		capacity_      = capacity;

		move_<0>(old_data, old_data + size_);

		deallocate_(old_data, size_);
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

#endif

