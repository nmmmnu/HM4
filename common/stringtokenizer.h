#ifndef STRING_TOKENIZER_H
#define STRING_TOKENIZER_H

#include "stringref.h"

#include <vector>
#include <cstdint>

class StringTokenizer{
private:
	static constexpr char DEFAULT_DELIMITER = ' ';

public:
	constexpr
	StringTokenizer(const StringRef &line, char const delimiter = DEFAULT_DELIMITER) :
						line_(line),
						delimiter_(delimiter){}

	inline void rewind(){
		pos_ = 0;
	}

	bool hasNext() const{
		return pos_ < line_.size();
	}

	StringRef getNext();

	std::vector<StringRef> getAll();

private:
	size_t		pos_ = 0;
	StringRef	line_;
	char		delimiter_;
};


#endif

