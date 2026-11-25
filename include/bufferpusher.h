#ifndef BUFFER_PUSHER_H_
#define BUFFER_PUSHER_H_

#include <cstring>
#include <cassert>

#include <string_view>

struct BufferPusher{
	BufferPusher(char *data) noexcept : data_(data){
		assert(data_);
	}

	void push(char c) noexcept{
		return push(std::string_view{ &c, 1 });
	}

	void push(std::string_view s) noexcept{
		memcpy(data_ + size_, s.data(), s.size());

		size_ += s.size();
	}

	void skip(size_t size) noexcept{
		size_ += size;
	}

	void skip(char c) noexcept{
		skip(std::string_view{ &c, 1 });
	}

	void skip(std::string_view s) noexcept{
		skip(s.size());
	}

	size_t size() const noexcept{
		return size_;
	}

private:
	char	*data_;
	size_t	size_ = 0;
};

#endif

