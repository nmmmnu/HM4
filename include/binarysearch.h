#ifndef BINARY_SEARCH_H_
#define BINARY_SEARCH_H_

#include <iterator>	// std::iterator_traits
#include <type_traits>

#include "comparator.h"

#include "software_prefetch.h"



auto binarySearchCompFn = [](auto const &a, auto const &b){
	return comparator::comp(a, b);
};

auto binarySearchPrefetchFn = [](auto const &){
};



template<
		class Iterator
>
struct BinarySearchResult{
	bool		found;
	Iterator	it;
};

#include "binarysearch.h.cc"

template <
		class Iterator								,
		class T									,
		class Comp		= decltype(binarySearchCompFn)			,
		class difference_type	= typename std::iterator_traits<Iterator>::difference_type
>
auto binarySearch(
		Iterator first, Iterator const &last					,
		T const &key								,
		Comp comp				= binarySearchCompFn		,
		difference_type const minimum_distance	= 5
) -> BinarySearchResult<Iterator>{
	using tag = typename std::iterator_traits<Iterator>::iterator_category;

	return binarySearch(std::move(first), last, key, comp, nullptr, minimum_distance, tag{});
}



template <
		class Iterator								,
		class T									,
		class Comp		= decltype(binarySearchCompFn)			,
		class Prefetch		= decltype(binarySearchPrefetchFn)		,
		class difference_type	= typename std::iterator_traits<Iterator>::difference_type
>
auto binarySearchPrefetch(
		Iterator first, Iterator const &last					,
		T const &key								,
		Comp comp				= binarySearchCompFn		,
		Prefetch prefetch			= binarySearchPrefetchFn	,
		difference_type const minimum_distance	= 5
) -> BinarySearchResult<Iterator>{
	using tag = typename std::iterator_traits<Iterator>::iterator_category;

	return binarySearch(std::move(first), last, key, comp, prefetch, minimum_distance, tag{});
}


#endif

