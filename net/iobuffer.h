#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H


#include "ambuffer.h"

#include <string_view>

#include <cstdio>
#include <cassert>

namespace net{

template<typename MemoryManager>
struct IOBuffer{
	using container_type	= am_buffer::AMBuffer<MemoryManager>;
	using size_type		= typename container_type::size_type;

private:
	constexpr static size_type KB			= 1024;
	constexpr static size_type STANDARD_READ_SIZE	= 4 * KB;

private:
	size_type		head_	= 0;
	container_type		buffer_;

public:
	IOBuffer(MemoryManager &mm			) noexcept : buffer_(mm){};

	IOBuffer(container_type &&buffer	) noexcept : buffer_(std::move(buffer)){}

	void clear(){
		buffer_.clear();
		head_ = 0;
	}

	void swap(container_type &buffer) noexcept{
		buffer_.swap(buffer);
	}

	void swap(IOBuffer &other){
		using std::swap;

		swap(head_	, other.head_	);
		swap(buffer_	, other.buffer_	);
	}

	container_type const &getBuffer() const{
		return buffer_;
	}

	container_type &getBuffer(){
		return buffer_;
	}

	// ==================================

	const char *data() const{
		const char *s = buffer_.data();
		return & s[head_];
	}

private:
	char *getIndex_(size_t index){
		return buffer_.data() + index;
	}

public:
	size_t size() const{
		return buffer_.size() - head_;
	}

	explicit operator std::string_view() const{
		return std::string_view{ data(), size() };
	}

	// ==================================

	[[nodiscard]]
	bool reserve(size_type const size){
		return buffer_.reserve(size);
	}

//	[[nodiscard]]
//	bool reserve(size_t const size){
//		return reserve(static_cast<size_type>(size));
//	}

	[[nodiscard]]
	size_t capacity() const{
		return buffer_.capacity();
	}

	// ==================================

//	[[nodiscard]]
//	bool push(const char c){
//		return buffer_.push(c);
//	}
//
	[[nodiscard]]
	bool push_(size_t const size, const char *ptr){
		if (!size)
			return false;

		return buffer_.push_(ptr, size);
	}

	[[nodiscard]]
	bool push_(std::string_view const s){
		return buffer_.push_(s.data(), s.size());
	}

	// ==================================

	[[nodiscard]]
	char *provideBuffer(size_t const size){
		assert(size && "Size must be great than zero");

		auto const oldSize = buffer_.size();

		if (!buffer_.resize(oldSize + size))
			return nullptr;

		return getIndex_(oldSize);
	}

	[[nodiscard]]
	auto provideBufferAtLeast(size_t const size){
		assert(size && "Size must be great than zero");

		struct Result{
			char	*buffer	= nullptr;
			size_t	size	= 0;
		};

		auto const oldSize = buffer_.size();

		if (!buffer_.resize(oldSize + size))
			return Result{};

		// now expand to the capacity
		if (!buffer_.resize(buffer_.capacity()))
			return Result{};

		return Result{
			getIndex_(oldSize),
			buffer_.size() - oldSize
		};
	}

	[[nodiscard]]
	auto provideBufferAtLeastStandardSize(){
		return provideBufferAtLeast(STANDARD_READ_SIZE);
	}

	void finalizeBuffer(const char *offset, size_t const actualSize){
		auto const size = static_cast<size_t>(offset - buffer_.data()) + actualSize;

		assert(size <= buffer_.capacity());

		[[maybe_unused]]
		bool const b = buffer_.resize(size);

		assert(b);
	}

	// ==================================

	bool pop(size_t const size){
		if (!size)
			return false;

		auto const availableSize = this->size();

		if (availableSize < size)
			return false;

		if (availableSize == size){
			clear();

			return true;
		}

		head_ += size;

		return true;
	}

	// ==================================

	void print(bool const body = true) const{
		printf("h: %10u | s: %10zu | c: %10zu | %.*s\n",
			head_		,
			size()		,
			capacity()	,
			(int) size()	,
			body ? data() : "[data]"
		);
	}
};



template<typename MemoryManager>
void swap(IOBuffer<MemoryManager> &a, IOBuffer<MemoryManager> &b){
	a.swap(b);
}

} // namespace

#endif

