#include "stringref.h"

#include "pair.h"

template <class List>
static int op_search(List const &list, StringRef const &key){
	if (key.empty())
		return 1;

	auto it = list.find(key, std::true_type{});

	if (it != std::end(list))
		print(*it);

	return 0;
}

// =====================================

template <class List>
static int op_iterate(List const &list, StringRef const &key, size_t const count = 10){
	size_t c = 0;

	auto it = key == '-' ? std::begin(list) : list.find(key, std::false_type{});

	for(; it != std::end(list); ++it){
		using hm4::print;

		print(*it);

		if (++c >= count)
			break;
	}

	return 0;
}

