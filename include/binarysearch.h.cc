template <
		class Iterator		,
		class T			,
		class Comp
>
auto linearSearch(
		Iterator first, Iterator const &last	,
		T const &key				,
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
		class Iterator		,
		class T			,
		class Comp		,
		class Prefetch		,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type
>
auto binarySearch(
		Iterator first, Iterator const &last	,
		T const &key				,
		Comp comp				,
		Prefetch	/* none */		,
		difference_type /* none */		,
		std::input_iterator_tag
){
	return linearSearch(std::move(first), last, key, comp);
}





template <
		class Iterator		,
		class T			,
		class Comp		,
		class Prefetch		,
		class difference_type = typename std::iterator_traits<Iterator>::difference_type
>
auto binarySearch(
		Iterator const &first, Iterator const &last	,
		T const &key					,
		Comp comp					,
		Prefetch user_prefetch				,
		difference_type const minimum_distance		,
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


		if constexpr( ! std::is_same_v<Prefetch, std::nullptr_t> ){
			auto const start1 = mid + 1;
			auto const end1 = mid;

			builtin_prefetch( & array[ start1 + ((end  - start1) >> 1) ], 0, 1);
			builtin_prefetch( & array[ start  + ((end1 - start ) >> 1) ], 0, 1);

			user_prefetch(      array[ start1 + ((end  - start1) >> 1) ] );
			user_prefetch(      array[ start1 + ((end  - start1) >> 1) ] );
		}

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

