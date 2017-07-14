#ifndef BINARY_SEARCH_H_
#define BINARY_SEARCH_H_

struct BinarySearchCompStdandard;	// [] <=>
struct BinarySearchCompList;		// cmpAt(index, key)

template <class ARRAY, class SIZE, class KEY, class COMP>
bool binarySearch(const ARRAY &list,
				SIZE start, SIZE end,
				const KEY &key,
				const COMP &comp,
				SIZE &result,
				SIZE minimum_distance = 5);

#include "binarysearch_impl.h"

#endif

