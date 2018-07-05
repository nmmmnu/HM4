#include "comparator.h"

template <class ARRAY, class SIZE, class KEY, class COMP>
BinarySearchResult<SIZE> binarySearch(const ARRAY &list,
				SIZE start, SIZE end,
				const KEY &key,
				const COMP &comp,
				SIZE const minimum_distance){
	/*
	 * Lazy based from Linux kernel...
	 * http://lxr.free-electrons.com/source/lib/bsearch.c
	 */
	while (start + minimum_distance < end){
		SIZE const mid = SIZE(start + ((end - start) >> 1));

		int const cmp = comp(list, mid, key);

		if (cmp < 0){
			// go right
			start = mid + 1;
		}else if (cmp > 0){
			// go left
			end = mid;
		}else{
			// found
			// index = mid
			return { true, mid };
		}

		//log__(start, end, end - start);
	}

	// fallback to linear search...
	for(; start < end; ++start){
		int const cmp = comp(list, start, key);

		if (cmp == 0){
			// found
			// index = left
			return { true, start };
		}

		if (cmp > 0)
			break;
	}

	return { false, start };
}

// ===================================

struct BinarySearchCompStdandard{
	template <class ARRAY, class SIZE, class KEY>
	int operator()(const ARRAY &list, SIZE const index, const KEY &key) const{
		return comparator::comp(list[index], key);
	}
};


