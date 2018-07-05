#ifndef BINARY_SEARCH_H_
#define BINARY_SEARCH_H_

#include <utility>	// std::pair

struct BinarySearchCompStdandard;	// [] <=>

template<class SIZE>
struct BinarySearchResult{
	bool	found;
	SIZE	pos;
};

template <class ARRAY, class SIZE, class KEY, class COMP>
BinarySearchResult<SIZE> binarySearch(const ARRAY &list,
				SIZE start, SIZE end,
				const KEY &key,
				const COMP &comp,
				SIZE minimum_distance = 5);

#include "binarysearch.h.cc"

#endif

