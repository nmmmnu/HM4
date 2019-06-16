#ifndef STRING_TOKENIZER_H
#define STRING_TOKENIZER_H

#include "stringref.h"

#include <vector>

#include <type_traits>



class StringTokenizer{
private:
	static constexpr char DEFAULT_DELIMITER = ' ';

public:
	class StringTokenizerIterator;

	constexpr
	StringTokenizer(const StringRef &line, char const delimiter = DEFAULT_DELIMITER) :
						line_(line),
						delimiter_(delimiter){}

	StringTokenizerIterator begin() const;
	StringTokenizerIterator end() const;

private:
	StringRef	line_;
	char		delimiter_;
};



class StringTokenizer::StringTokenizerIterator{
public:
	using difference_type = ptrdiff_t;
	using value_type = StringRef;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	StringTokenizerIterator(const char *pos, const char *last, char const delimiter):
							pos_(pos),
							last_(last),
							delimiter_(delimiter){}

	StringRef operator *() const{
		auto const size = skipData_() - pos_;
		return { pos_, size_t(size) };
	}

	bool operator ==(StringTokenizerIterator const &other) const{
		return
			pos_		== other.pos_		&&
			last_		== other.last_		&&
			delimiter_	== other.delimiter_
		;
	}

	bool operator !=(StringTokenizerIterator const &other) const{
		return ! (*this == other);
	}

	StringTokenizerIterator &operator ++(){
		pos_ = skipDataAndDelimiter_();

		return *this;
	}

private:
	const char *skipData_() const{
		const char *it = pos_;

		while(it != last_ && *it != delimiter_)
			++it;

		return it;
	}

	const char *skipDataAndDelimiter_() const{
		const char *it = skipData_();

		while(it != last_ && *it == delimiter_)
			++it;

		return it;
	}

private:
	const char *pos_;
	const char *last_;

	char delimiter_;
};



inline StringTokenizer::StringTokenizerIterator StringTokenizer::begin() const{
	return { std::begin(line_), std::end(line_), delimiter_ };
}

inline StringTokenizer::StringTokenizerIterator StringTokenizer::end() const{
	return { std::end(line_),   std::end(line_), delimiter_ };
}



inline StringRef getNextToken(StringTokenizer::StringTokenizerIterator &it, StringTokenizer::StringTokenizerIterator const &end){
	StringRef s;

	if (it == end)
		return s;

	s = *it;

	++it;

	return s;
}


#if 0

#include <utility>

inline auto getPair(StringTokenizer::StringTokenizerIterator &it, StringTokenizer::StringTokenizerIterator const &end){
	StringRef k = getNext(it, end);
	StringRef v = getNext(it, end);

	return std::make_pair(k, v);
}

#endif


#endif

