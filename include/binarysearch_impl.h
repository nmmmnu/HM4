#include "comparator.h"

template <class ARRAY, class SIZE, class KEY, class COMP>
std::pair<bool,SIZE> binarySearch(const ARRAY &list,
				SIZE start, SIZE end,
				const KEY &key,
				const COMP &comp,
				SIZE const minimum_distance){
	/*
	 * Lazy based from Linux kernel...
	 * http://lxr.free-electrons.com/source/lib/bsearch.c
	 */

	while (start + minimum_distance < end){
	//	SIZE const mid = start + ((end - start) /  2);
		SIZE const mid = SIZE(start + ((end - start) >> 1)); // 4% faster

		int const cmp = comp(list, mid, key); //list.cmpAt(mid, key);

		if (cmp < 0){
			// go right
			start = mid + 1;

			// scatter code would go here.
			// but it have negative effect.
		}else if (cmp > 0){
			// go left
			end = mid;
		}else{
			// found
			// index = mid; return 0;
			return { true, mid };
		}

	}

	// fallback to linear search...
	for(; start < end; ++start){
		int const cmp = comp(list, start, key); //list.cmpAt(start, key);

		if (cmp == 0){
			// found
			// index = left; return 0;
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

struct BinarySearchCompList{
	template <class ARRAY, class SIZE, class KEY>
	int operator()(const ARRAY &list, SIZE const index, const KEY &key) const{
		return list.cmpAt(index, key);
	}
};

