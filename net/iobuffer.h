#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H

#include <vector>
#include <string_view>

#include <cstdio>
#include <cassert>

namespace net{

class IOBuffer{
private:
	using container_type	= std::vector<char>;
	using size_type		= container_type::size_type;

	constexpr static size_t INITIAL_RESERVE = 64;

private:
	size_type		head_	= 0;
	container_type		buffer_;

public:
	IOBuffer(size_t const reserve = INITIAL_RESERVE){
		buffer_.reserve(reserve);
	}

	void clear(){
		buffer_.clear();
		head_ = 0;
	}

	void swap(IOBuffer &other){
		using std::swap;
		swap(head_	, other.head_	);
		swap(buffer_	, other.buffer_	);
	}

	// ==================================

	const char *data() const{
		const char *s = buffer_.data();
		return & s[head_];
	}

	size_t size() const{
		return buffer_.size() - head_;
	}

	explicit operator std::string_view(){
		return std::string_view{ data(), size() };
	}

	// ==================================

	void reserve(container_type::size_type const size){
		buffer_.reserve(size);
	}

	auto capacity() const{
		return buffer_.capacity();
	}

	// ==================================

	bool push(const char c){
		push_(c);
		return true;
	}

	bool push(std::string_view const sv){
		if (sv.size())
			return push_(sv.size(), sv.data());

		return false;
	}

	bool push(size_t const len, const char *ptr){
		if (ptr && len)
			return push_(len, ptr);

		return false;
	}

	bool pop(size_t const len){
		if (len == 0)
			return false;

		auto const available = size();

		if (available < len)
			return false;

		if (available == len){
			clear();
			return true;
		}

		head_ = head_ + len;

		return true;
	}

	// ==================================

	void print() const{
		printf("h: %3zu | s: %3zu | %.*s\n",
				head_,
				size(),
				(int) size(), buffer_.data() );
	}

private:
	bool push_(size_t const len, const char *ptr){
		assert(ptr);
		assert(len);

		buffer_.insert(std::end(buffer_), ptr, ptr + len);

		return true;
	}

	bool push_(const char c){
		buffer_.push_back(c);
		return true;
	}
};



void swap(IOBuffer &a, IOBuffer &b){
	a.swap(b);
}



} // namespace

#endif

