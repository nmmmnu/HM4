template <
		class Iterator,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type,
		class T,
		class Comp
>
auto linearSearch(
		Iterator const begin, Iterator const end,
		T const &key,
		Comp const &comp
) -> BinarySearchResult<Iterator>{
	difference_type pos = 0;
	auto it = begin;

	for(;it != end; ++it, ++pos){
		int const cmp = comp(*it, key);

		if (cmp == 0){
			// found
			// index = pos
			return { true, pos, it };
		}

		if (cmp > 0)
			break;
	}

	return { false, pos, it };
}





template <
		class Iterator,
		class difference_type,
		class T,
		class Comp
>
auto binarySearch(
		Iterator const begin, Iterator const end,
		T const &key,
		Comp const &comp,
		difference_type,
		std::input_iterator_tag
) -> BinarySearchResult<Iterator>{
	return linearSearch(begin, end, key, comp);
}





template <
		class Iterator,
		class difference_type,
		class T,
		class Comp
>
auto binarySearch(
		Iterator const it_begin, Iterator const it_end,
		T const &key,
		Comp const &comp,
		difference_type const minimum_distance,
		std::random_access_iterator_tag
) -> BinarySearchResult<Iterator>{
	/*
	 * Lazy based from Linux kernel...
	 * http://lxr.free-electrons.com/source/lib/bsearch.c
	 */
	difference_type start = 0;
	difference_type end   = it_end - it_begin;

	Iterator const &array = it_begin;

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
			return { true, mid, array + mid };
		}
	}

	// fallback to linear search...
	return linearSearch(array + start, array + end, key, comp);
}

