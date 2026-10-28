#ifndef MY_FAST_BUFFER_H_
#define MY_FAST_BUFFER_H_

#include <cstring>
#include <cstdlib>	// free

#include <new>		// std::bad_alloc
#include <utility>	// std::swap

class MyFastBuffer{
	char	*data_		= nullptr;	// realloc is OK with nullptr
	size_t	size_		= 0;
	size_t	capacity_ 	= 0;

public:
	using size_type		= size_t;

private:
	constexpr static size_type MULTIPLIER		= 2;
	constexpr static size_type INITIAL_RESERVE	= 4096;

public:
	explicit MyFastBuffer(size_t capacity = 0){
		reserve(capacity ? capacity : INITIAL_RESERVE);
	}

	~MyFastBuffer(){
		free(data_);
	}

	MyFastBuffer(const MyFastBuffer &) = delete;
	MyFastBuffer &operator=(const MyFastBuffer &) = delete;

	constexpr MyFastBuffer(MyFastBuffer &&other) noexcept:
					data_		(other.data_		),
					size_		(other.size_		),
					capacity_	(other.capacity_	){
		other.data_ = nullptr;
	}

	constexpr MyFastBuffer &operator=(MyFastBuffer &&other) noexcept{
		if (this == &other)
			return *this;

		data_		= other.data_		;
		size_		= other.size_		;
		capacity_	= other.capacity_	;

		other.data_ = nullptr;

		return *this;
	}

	void swap(MyFastBuffer &other) noexcept{
		using std::swap;

		swap(data_	, other.data_		);
		swap(size_	, other.size_		);
		swap(capacity_	, other.capacity_	);
	}

public:
	constexpr char *data(){
		return data_;
	}

	constexpr const char *data() const{
		return data_;
	}

	constexpr size_t size() const{
		return size_;
	}

	constexpr size_t capacity() const{
		return capacity_;
	}

public:
	constexpr void clear(){
		size_ = 0;
	}

	void reserve(size_t newCapacity){
		if (newCapacity <= capacity_)
			return;

		void *newData = realloc(data_, newCapacity);

		if (!newData)
			throw std::bad_alloc();

		data_		= static_cast<char *>(newData);
		capacity_	= newCapacity;
	}

	void resize(size_t newSize){
		if (newSize > capacity_)
			reserveCheck_(newSize - capacity_);

		size_ = newSize;
	}

	void push(char c){
		reserveCheck_(1);

		data_[size_++] = c;
	}

	char *push(size_t size){
		reserveCheck_(size);

		auto *dst = data_ + size_;

		size_ += size;

		return dst;
	}

	void push(const char *src, size_t size){
		char *dst = push(size);

		memcpy(dst, src, size);
	}

private:
	void reserveCheck_(size_t size){
		if (size_ + size > capacity_)
			reserve((size_ + size) * MULTIPLIER);
	}
};

void swap(MyFastBuffer &a, MyFastBuffer &b) noexcept{
	a.swap(b);
}

#endif

