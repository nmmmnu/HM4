#include "stringtokenizer.h"

#include <cassert>

StringRef StringTokenizer::getNext(){
	size_t start = pos_;

	while(hasNext() && line_[pos_] != delimiter_)
		++pos_;

	size_t size = pos_ - start;

	StringRef s{ &line_[start], size };

	while(hasNext() && line_[pos_] == delimiter_)
		++pos_;

	return s;
}

std::vector<StringRef> StringTokenizer::getAll(){
	rewind();

	std::vector<StringRef> v;

	while(hasNext()){
		v.push_back(getNext());
	}

	return v;
}

