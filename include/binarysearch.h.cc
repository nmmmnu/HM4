template <
		class Iterator,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type,
		class T,
		class Comp
>
auto linearSearch(
		Iterator first, Iterator const &last,
		T const &key,
		Comp comp
) -> BinarySearchResult<Iterator>{
	for(; first != last; ++first){
		int const cmp = comp(*first, key);

		if (cmp == 0){
			// found
			// index = pos
			return { true, first };
		}

		if (cmp > 0)
			break;
	}

	return { false, first };
}





template <
		class Iterator,
		class difference_type,
		class T,
		class Comp
>
auto binarySearch(
		Iterator first, Iterator const &last,
		T const &key,
		Comp comp,
		difference_type,
		std::input_iterator_tag
){
	return linearSearch(std::move(first), last, key, comp);
}





template <
		class Iterator,
		class difference_type,
		class T,
		class Comp
>
auto binarySearch(
		Iterator const &first, Iterator const &last,
		T const &key,
		Comp comp,
		difference_type const minimum_distance,
		std::random_access_iterator_tag
) -> BinarySearchResult<Iterator>{
	/*
	 * Lazy based from Linux kernel...
	 * http://lxr.free-electrons.com/source/lib/bsearch.c
	 */
	auto start = difference_type{ 0 };
	auto end   = last - first;

	Iterator const &array = first;

	while (start + minimum_distance < end){
		difference_type const mid = static_cast<difference_type>( start + ((end - start) >> 1) );

		int const cmp = comp(array[mid], key);

		if (cmp < 0){
			// go right
			start = mid + 1;
		}else if (cmp > 0){
			// go left
			end = mid;
		}else{
			// found
			// index = mid
			return { true, array + mid };
		}
	}

	// fallback to linear search...
	return linearSearch(array + start, array + end, key, comp);
}

