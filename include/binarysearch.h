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

// version with all available options
template <typename Iterator, typename TKey, class Comp, class Prefetch>
auto binarySearchPrefetch(
		Iterator	first		,
		Iterator const	&last		,
		TKey const	&key		,
		Comp		&&comp		,
		Prefetch	&&prefetch	,
		typename std::iterator_traits<Iterator>::difference_type const minimum_distance	= 5
){
	using tag = typename std::iterator_traits<Iterator>::iterator_category;

	return binary_search_impl_::binarySearch(
			std::move(first), last,
			key					,
			std::forward<Comp>(comp)		,
			std::forward<Prefetch>(prefetch)	,
			minimum_distance			,
			tag{}
	);
}

// version with no prefetch
template <typename Iterator, typename TKey, class Comp>
auto binarySearch(
		Iterator	first	,
		Iterator const	&last	,
		TKey const	&key	,
		Comp		&&comp	,
		typename std::iterator_traits<Iterator>::difference_type const minimum_distance = 5
){
	return binarySearchPrefetch(
			std::move(first), last		,
			key				,
			std::forward<Comp>(comp)	,
			binarySearchPrefetchFn		,
			minimum_distance
	);
}

// version with no comp
template <typename Iterator, typename TKey>
auto binarySearch(
		Iterator	first	,
		Iterator const	&last	,
		TKey const	&key	,
		typename std::iterator_traits<Iterator>::difference_type const minimum_distance = 5
){
	return binarySearchPrefetch(
			std::move(first), last		,
			key				,
			binarySearchCompFn		,
			binarySearchPrefetchFn		,
			minimum_distance
	);
}

#endif

