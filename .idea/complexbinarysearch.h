#ifndef COMPLEX_BINARY_SEARCH_H_
#define COMPLEX_BINARY_SEARCH_H_



template<typename difference_type>
struct ComplexBinarySearchResult{
	bool		found;
	difference_type	pos;
};



template <
		typename difference_type,
		typename T,
		typename Comp
>
auto complexBinarySearch(
		difference_type	start	,
		difference_type	end	,
		T const		&key	,
		Comp		comp
) -> ComplexBinarySearchResult<difference_type>{
	/*
	 * Lazy based from Linux kernel...
	 * http://lxr.free-electrons.com/source/lib/bsearch.c
	 */

	while (start < end){
		difference_type mid = static_cast<difference_type>( start + ((end - start) >> 1) );

		int const cmp = comp(key, start, mid, end);

		// mid can be changed...

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
	}

	return { false, end };
}



template <
		typename difference_type,
		typename T,
		typename Comp
>
auto complexBinarySearch(
		difference_type	end	,
		T const		&key	,
		Comp		comp
){
	return complexBinarySearch(difference_type{ 0 }, end, key, comp);
}



#endif

