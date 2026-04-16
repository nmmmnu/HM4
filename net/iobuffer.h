#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H

#include "myfastbuffer.h"

#include <string_view>

#include <cstdio>
#include <cassert>

namespace net{

struct IOBuffer{
	using container_type	= MyFastBuffer;
	using size_type		= container_type::size_type;

	constexpr static size_type INITIAL_RESERVE = 4096;

private:
	size_type		head_	= 0;
	container_type		buffer_;

public:
	IOBuffer(size_type const size = INITIAL_RESERVE) : buffer_(size){
	}

	IOBuffer(container_type &&buffer) noexcept : buffer_(std::move(buffer)){
	}

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

	void reserve(size_type const size){
		buffer_.reserve(size);
	}

	auto capacity() const{
		return buffer_.capacity();
	}

	// ==================================

	bool push(const char c){
		buffer_.push(c);
		return true;
	}

	bool push(std::string_view const s){
		buffer_.push(s.data(), s.size());
		return true;
	}

	bool push(size_t const size, const char *ptr){
		if (!size)
			return false;

		buffer_.push(ptr, size);

		return true;
	}

	template<class Lazy>
	bool push(std::false_type, size_t const size, Lazy f){
		if (!size)
			return false;

		char *dest = buffer_.push(size);

		f(dest);

		// no need to finalizeWriteBuffer

		return true;
	}

#if 0
	template<class Lazy>
	bool push(std::true_type, size_t const desiredSize, Lazy f){
		assert(desiredSize && "Size must be great than zero");

		char *dest = buffer_.push(desiredSize);

		auto const finalSize = f(dest);

		return finalizeWriteBuffer(dest, finalSize);
	}
#endif

	// ==================================

	char *provideWriteBuffer(size_t const size){
		assert(size && "Size must be great than zero");

		auto const oldSize = buffer_.size();

		buffer_.resize(oldSize + size);

		return getIndex_(oldSize);
	}

	auto provideWriteBufferAtLeast(size_t const size){
		assert(size && "Size must be great than zero");

		struct Result{
			char	*buffer;
			size_t	size;
		};

		auto const oldSize = buffer_.size();

		buffer_.resize(oldSize + size);
		buffer_.resize(buffer_.capacity());

		return Result{
			getIndex_(oldSize),
			buffer_.size() - oldSize
		};
	}

	bool finalizeWriteBuffer(const char *offset, size_t const actualSize){
		buffer_.resize(offset - buffer_.data() + actualSize);

		return true;
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
		printf("h: %10zu | s: %10zu | c: %10zu | %.*s\n",
			head_		,
			size()		,
			capacity()	,
			(int) size()	,
			body ? data() : "[data]"
		);
	}
};



void swap(IOBuffer &a, IOBuffer &b){
	a.swap(b);
}

} // namespace

#endif

