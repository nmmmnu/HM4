#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H

#include <string>
#include <string_view>

#include <cstdio>
#include <cassert>

namespace net{

class IOBuffer{
private:
	using container_type	= std::string;
	using size_type		= container_type::size_type;

private:
	size_type		head_	= 0;
	container_type		buffer_;

public:
	void clear(){
		buffer_.clear();
		head_ = 0;
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

	bool push(const char c){
		push_(c);
		return true;
	}

	bool push(const char *p){
		return push(std::string_view{ p });
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

		buffer_.append(ptr, len);

		return true;
	}

	bool push_(const char c){
		buffer_.push_back(c);
		return true;
	}
};

} // namespace

#endif

