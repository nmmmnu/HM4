#ifndef _IO_BUFFER_H
#define _IO_BUFFER_H

#include <vector>
#include <string_view>

#include <limits>

#include <cstring>
#include <cstdio>
#include <cassert>

namespace net{

class IOBuffer{
public:
	using container_type	= std::vector<char>;
	using size_type		= container_type::size_type;

	constexpr static size_type INITIAL_RESERVE = 64;

private:
	size_type		head_	= 0;
	container_type		buffer_;

public:
	IOBuffer(size_type const size = INITIAL_RESERVE){
		reserve(size);
	}

	IOBuffer(container_type &&buffer) : buffer_(std::move(buffer)){
	}

//	IOBuffer(container_type &buffer){
//		swap(buffer_, buffer);
//	}

	void clear(){
		buffer_.clear();
		head_ = 0;
	}

	void swap(container_type &buffer){
		using std::swap;
		swap(buffer_	, buffer	);
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

	size_t size() const{
		return buffer_.size() - head_;
	}

	explicit operator std::string_view(){
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
		buffer_.push_back(c);
		return true;
	}

	bool push(std::string_view const sv){
		return push(sv.size(), sv.data());
	}

	bool push(size_t const len, const char *ptr){
		if (!len)
			return false;

		if constexpr(false){
			buffer_.insert(std::end(buffer_), ptr, ptr + len);
			return true;
		}else{
			return push(std::false_type{}, len, [ptr,len](void *dest){
				memcpy(dest, ptr, len);

				return len;
			});
		}
	}

	template<class Lazy>
	bool push(std::true_type, size_t const desired_len, Lazy f){
		if (!desired_len)
			return false;

		auto const size = buffer_.size();

		buffer_.resize(size + desired_len);

		char *dest = & buffer_[size];

		auto const final_len = f(dest);

		buffer_.resize(size + final_len);

		return true;
	}

	template<class Lazy>
	bool push(std::false_type, size_t const len, Lazy f){
		if (!len)
			return false;

		auto const size = buffer_.size();

		buffer_.resize(size + len);

		char *dest = & buffer_[size];

		f(dest);

		return true;
	}

	// ==================================

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

	void print(bool const body = true) const{
		printf("h: %10zu | s: %10zu | c: %10zu | %.*s\n",
			head_		,
			size()		,
			capacity()	,
			(int) size()	,
			body ? buffer_.data() : "[data]"
		);
	}
};



void swap(IOBuffer &a, IOBuffer &b){
	a.swap(b);
}



} // namespace

#endif

