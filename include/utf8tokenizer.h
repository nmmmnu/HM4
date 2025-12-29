#ifndef UTF8_TOKENIZER_H_
#define UTF8_TOKENIZER_H_

#include <string_view>
#include <iterator>

class ASCIITokenizer{
	std::string_view s_;

public:
	using iterator = const char *;

	constexpr ASCIITokenizer(std::string_view s) : s_(s){}

	constexpr iterator begin() const{
		return s_.data();
	}

	constexpr iterator end() const{
		return s_.data() + s_.size();
	}

	constexpr std::string_view to(iterator it) const{
		return s_.substr(0, psize_(it));
	}

#if 0
	constexpr std::string_view from(iterator it, size_t size) const{
		const auto start_offset = it - begin();

		if (static_cast<size_t>(start_offset) >= s_.size())
			return {};

		const size_t remaining = s_.size() - start_offset;
		const size_t len = size < remaining ? size : remaining;

		return s_.substr(start_offset, len);
	}
#endif

private:
	constexpr size_t psize_(iterator it) const{
		return it - begin() + 1;
	}
};

using BinaryTokenizer = ASCIITokenizer;



struct UTF8Tokenizer{
	constexpr static size_t MAX_UTF8_SIZE = 4;

private:
	std::string_view s_;

	class Iterator;

public:
	using iterator = Iterator;

	constexpr UTF8Tokenizer(std::string_view s) : s_(s){}

	constexpr iterator begin() const;
	constexpr iterator end()   const;

	constexpr std::string_view to(iterator const &it) const{
		return s_.substr(0, psize_(it));
	}

	// constexpr std::string_view from(iterator const &it, size_t size) const;

private:
	constexpr size_t psize_(iterator const &it) const;
};





class UTF8Tokenizer::Iterator{
	std::string_view	s_;
	std::string_view	c_;
	size_t			p_;

public:
	using value_type        = std::string_view;
	using reference         = std::string_view;
	using difference_type   = std::ptrdiff_t;
	using iterator_category = std::input_iterator_tag;

	constexpr Iterator(std::string_view s, size_t pos) : s_(s), p_(pos){
		updateCurrent_();
	}

	constexpr reference operator*() const{
		return c_;
	}

	constexpr Iterator &operator++(){
		if (p_ < s_.size()){
			p_ += getCurrentCharLen_();
			updateCurrent_();
		}

		return *this;
	}

	constexpr bool operator!=(Iterator const &other) const{
		return p_ != other.p_ || s_.data() != other.s_.data();
	}

	constexpr bool operator==(Iterator const &other) const{
		return ! operator!=(other);
	}

	constexpr auto pos() const{
		return p_;
	}

	constexpr auto posSize() const{
		return c_.size();
	}

	constexpr auto posFinish() const{
		return p_ + c_.size();
	}

#if 0
	constexpr std::string_view from(size_t size) const{
		auto const begin     = p_;

		if (begin >= s_.size())
			return {};

		auto end = begin;

		while(size-- && end < s_.size()){
			auto const len = getCharLen__(s_[end]);

			if (end + len > s_.size())
				break;

			end += len;
		}

		return s_.substr(begin, end - begin);
	}
#endif

private:
	constexpr void updateCurrent_(){
		if (p_ < s_.size()){
			auto const len1 = getCurrentCharLen_();
			auto const len  = p_ + len1 > s_.size() ? 1 : len1;

			c_ = s_.substr(p_, len);
		}else{
			// we do not care
			// c_ = {};
		}
	}

	constexpr size_t getCurrentCharLen_() const{
		return getCharLen__(s_[p_]);
	}

	constexpr static size_t getCharLen__(char cc){
		auto const c = static_cast<unsigned char>(cc);

		if (c < 0x80)
			return 1;	// 1-byte ASCII

		if ((c >> 5) == 0x06)
			return 2;	// 110xxxxx -> 2 bytes

		if ((c >> 4) == 0x0E)
			return 3;	// 1110xxxx -> 3 bytes

		if ((c >> 3) == 0x1E)
			return 4;	// 11110xxx -> 4 bytes

		return 1;		// invalid byte
	}
};

constexpr auto UTF8Tokenizer::begin() const -> iterator{
	return { s_, 0 };
}

constexpr auto UTF8Tokenizer::end() const -> iterator{
	return { s_, s_.size() };
}

constexpr size_t UTF8Tokenizer::psize_(iterator const &it) const{
	return it.posFinish();
}

// constexpr std::string_view UTF8Tokenizer::from(iterator const &it, size_t size) const{
// 	return it.from(size);
// }

#endif

