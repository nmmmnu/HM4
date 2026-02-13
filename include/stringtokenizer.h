#ifndef STRING_TOKENIZER_H
#define STRING_TOKENIZER_H

#include <string_view>
#include <cstddef>

class StringTokenizer{
private:
	static constexpr char DEFAULT_DELIMITER = ' ';

public:
	class iterator;

	constexpr
	StringTokenizer(std::string_view const line, char delimiter = DEFAULT_DELIMITER) :
						line_(line),
						delimiter_(delimiter){}

	iterator begin() const;
	iterator end() const;

private:
	std::string_view line_;
	char delimiter_;
};



class StringTokenizer::iterator{
public:
	using difference_type	= ptrdiff_t;
	using value_type	= std::string_view const;
	using pointer		= value_type *;
	using reference		= value_type; // is string_view, copy is cheap
	using iterator_category	= std::bidirectional_iterator_tag;

public:
	iterator(const char *current, const char *first, const char *last, char const delimiter):
							current_(current),
							first_(first),
							last_(last),
							delimiter_(delimiter){}

	std::string_view operator *() const{
		auto size = skipData_(current_) - current_;

		return { current_, size_t(size) };
	}

	bool operator ==(iterator const &other) const{
		return
			current_	== other.current_	&&
			first_		== other.first_		&&
			last_		== other.last_		&&
			delimiter_	== other.delimiter_
		;
	}

	bool operator !=(iterator const &other) const{
		return ! (*this == other);
	}

	iterator &operator ++(){
		current_ = skipData_(current_);
		current_ = skipDelimiter_(current_);

		return *this;
	}

	iterator &operator --(){
		current_ = skipDelimiterRev_(current_);
		current_ = skipDataRev_(current_);

		return *this;
	}

private:
	const char *skipData_(const char *it) const{
		while(it != last_ && *it != delimiter_)
			++it;

		return it;
	}

	const char *skipDelimiter_(const char *it) const{
		if (it != last_ && *it == delimiter_)
			++it;

		return it;
	}

private:
	const char *skipDataRev_(const char *it) const{
		while(it != first_ && *(it - 1) != delimiter_)
			--it;

		return it;
	}

	const char *skipDelimiterRev_(const char *it) const{
		if (it != first_ && *(it - 1) == delimiter_)
			--it;

		return it;
	}

private:
	const char *current_;
	const char *first_;
	const char *last_;

	char delimiter_;
};



inline auto StringTokenizer::begin() const -> iterator{
	return { std::begin(line_), std::begin(line_), std::end(line_), delimiter_ };
}

inline auto StringTokenizer::end() const -> iterator{
	return { std::end(line_),  std::begin(line_), std::end(line_), delimiter_ };
}



inline auto getForwardTokenizer(StringTokenizer const &st){
	return [ current = std::begin(st), sentinel = std::end(st) ]() mutable -> std::string_view{
		if (current == sentinel)
			return {};

		std::string_view s = *current;

		++current;

		return s;
	};
}

inline auto getReverseTokenizer(StringTokenizer const &st){
	return [ current = std::end(st), sentinel = std::begin(st) ]() mutable -> std::string_view{
		if (current == sentinel)
			return {};

		--current;

		std::string_view s = *current;

		return s;
	};
}

inline auto getBackwardTokenizer(StringTokenizer const &st){
	return getReverseTokenizer(st);
}

#endif

