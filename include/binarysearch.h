#ifndef BINARY_SEARCH_H_
#define BINARY_SEARCH_H_

#include <iterator>	// std::iterator_traits
#include <type_traits>

#include "comparator.h"



auto binarySearchComp = [](auto const a, auto const b){
	return comparator::comp(a, b);
};



template<
		class Iterator,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type
>
struct BinarySearchResult{
	bool		found;
	difference_type	pos;
	Iterator	it;
};

#include "binarysearch.h.cc"

template <
		class Iterator,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type,
		class T,
		class Comp = decltype(binarySearchComp)
>
auto binarySearch(
		Iterator const begin, Iterator const end,
		T const &key,
		Comp const &comp = binarySearchComp,
		difference_type const minimum_distance = 5
) -> BinarySearchResult<Iterator>{
	using tag = typename std::iterator_traits<Iterator>::iterator_category;

	return binarySearch(begin, end, key, comp, minimum_distance, tag{});
}


#endif

