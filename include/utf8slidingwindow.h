#ifndef UTF8_SLIDING_WINDOW_H_
#define UTF8_SLIDING_WINDOW_H_

#include <type_traits>

#include "utf8tokenizer.h"



class ASCIISlidingWindow{
    std::string_view	s_	;
    size_t		size_	;

public:
	ASCIISlidingWindow(std::string_view s, size_t size) : s_(s), size_(size){}

	class Iterator;

	Iterator begin() const;
	Iterator end()   const;
};

class ASCIISlidingWindow::Iterator{
	std::string_view s_;
	size_t pos_ = 0;
	size_t size_;

public:
	using value_type      = std::string_view;
	using reference       = std::string_view;
	using difference_type = std::ptrdiff_t;

	Iterator(std::string_view s, size_t pos, size_t size) :
						s_	(s	),
						pos_	(pos	),
						size_	(size	){}

	reference operator*() const{
		if (pos_ + size_ <= s_.size())
			return s_.substr(pos_, size_);
		else
			return s_.substr(pos_);
	}

	Iterator &operator++(){
		++pos_;
		return *this;
	}

	bool operator!=(Iterator const &other) const{
		return pos_ != other.pos_;
	}
};

auto ASCIISlidingWindow::begin() const -> Iterator{
	return Iterator(s_, 0,         size_);
}

auto ASCIISlidingWindow::end()   const -> Iterator{
	return Iterator(s_, s_.size(), size_);
}

using BinarySlidingWindow = ASCIISlidingWindow;





class UTF8SlidingWindow {
	std::string_view	s_	;
	size_t			size_	;

public:
	constexpr UTF8SlidingWindow(std::string_view s, size_t size) : s_(s), size_(size){}

	class Iterator;

	Iterator begin() const;
	Iterator end()   const;
};

class UTF8SlidingWindow::Iterator{
	using TokIterator = typename UTF8Tokenizer::iterator;

	std::string_view	s_	;
	TokIterator		it1_	;
	TokIterator		it2_	;
	TokIterator		end_	;

	template<bool B>
	constexpr Iterator(std::bool_constant<B>, std::string_view s, size_t size, TokIterator it, TokIterator et) :
						s_	(s	),
						it1_	(it	),
						it2_	(it	),
						end_	(et	){
		if constexpr(B){
			while(--size && it2_ != end_)
				++it2_;
		}
	}

	constexpr Iterator(std::true_type  tag, std::string_view s, size_t size, UTF8Tokenizer t) :
					Iterator(tag, s, size, std::begin(t), std::end(t)){}

	constexpr Iterator(std::false_type tag, std::string_view s, size_t size, UTF8Tokenizer t) :
					Iterator(tag, s, size, std::end(t),   std::end(t)){}

public:
	using value_type	= std::string_view		;
	using reference		= std::string_view		;
	using difference_type	= std::ptrdiff_t		;

	template<bool B>
	constexpr Iterator(std::bool_constant<B> tag, std::string_view s, size_t size) : Iterator(tag, s, size, {s}){}

	constexpr reference operator*() const{
		return s_.substr(it1_.pos(), it2_.posFinish() - it1_.pos());
	}

	constexpr Iterator &operator++(){
		if (it1_ != end_)
			++it1_;

		if (it2_ != end_)
			++it2_;

		return *this;
	}

	constexpr bool operator!=(Iterator const &other) const{
		return it1_ != other.it1_ || it2_ != other.it2_;
	}
};

auto UTF8SlidingWindow::begin() const -> Iterator{
	return Iterator{ std::true_type{},  s_, size_ };
}

auto UTF8SlidingWindow::end()   const -> Iterator{
	return Iterator{ std::false_type{}, s_, size_ };
}



#endif

