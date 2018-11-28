
template <
		class Iterator,
		class T,
		class Comp,
		class difference_type
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
	difference_type i = start;
	for(i = start; i < end; ++i){
		int const cmp = comp(array[i], key);

		if (cmp == 0){
			// found
			// index = i
			return { true, i, array + i };
		}

		if (cmp > 0)
			break;
	}

	return { false, i, array + i };
}




template <
		class Iterator,
		class T,
		class Comp,
		class difference_type
>
auto binarySearch(
		Iterator const begin, Iterator const end,
		T const &key,
		Comp const &comp,
		difference_type,
		std::input_iterator_tag
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



