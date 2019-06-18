#ifndef MY_STRING_TOKENIZER_H_
#define MY_STRING_TOKENIZER_H_

#include "Development/HM4/include/stringref.h"

#include <algorithm>	// std::find
#include <type_traits>

class MyStringTokenizerIterator{
public:
	using difference_type = ptrdiff_t;
	using value_type = const StringRef;
	using pointer = value_type *;
	using reference = value_type &;
	using iterator_category = std::forward_iterator_tag;

public:
	MyStringTokenizerIterator(const StringRef &s, char const delimiter, std::true_type) :
				delimiter_(delimiter),
				from_	(std::begin(s)),
				last_	(std::end(s)),
				to_	(advance_()){}

	MyStringTokenizerIterator(const StringRef &s, char const delimiter, std::false_type) :
				delimiter_(delimiter),
				from_	(std::end(s)),
				last_	(std::end(s)),
				to_	(std::end(s)){}

public:
	MyStringTokenizerIterator &operator ++(){
		from_ = to_;
		to_   = advance_();

		return *this;
	}

	StringRef operator *() const{
		return { from_, size_t(to_ - from_) };
	}

public:
	bool operator ==(MyStringTokenizerIterator const &other) const{
		return
			delimiter_	== other.delimiter_	&&
			from_		== other.from_		&&
			to_		== other.to_		&&
			last_		== other.last_;

	}

	bool operator !=(MyStringTokenizerIterator const &other) const{
		return ! (*this == other);
	}

private:
	const char *advance_(){
		if (from_ != last_ && *from_ == delimiter_)
			++from_;

		return std::find(from_, last_, delimiter_);
	}

private:
	char		delimiter_;

	const char	*from_;
	const char	*last_;
	const char	*to_;
};

class MyStringTokenizer{
public:
	constexpr MyStringTokenizer(const StringRef &data, char const delimiter):
					data_(data),
					delimiter_(delimiter){}

	MyStringTokenizerIterator begin() const{
		return { data_, delimiter_, std::true_type{} };
	}

	MyStringTokenizerIterator end() const{
		return { data_, delimiter_, std::false_type{} };
	}

private:
	const StringRef	&data_;
	char		delimiter_;
};

#endif



#include <iostream>

int main(){
	const StringRef s = "www.nmmm.co.uk";

	for(const auto &s : MyStringTokenizer{ s, '.' })
		std::cout << s << '\n';
}

