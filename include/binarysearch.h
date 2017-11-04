#ifndef BINARY_SEARCH_H_
#define BINARY_SEARCH_H_

#include <utility>	// std::pair

struct BinarySearchCompStdandard;	// [] <=>

template <class ARRAY, class SIZE, class KEY, class COMP>
std::pair<bool,SIZE> binarySearch(const ARRAY &list,
				SIZE start, SIZE end,
				const KEY &key,
				const COMP &comp,
				SIZE minimum_distance = 5);

#include "binarysearch.h.cc"

#endif

