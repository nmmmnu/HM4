#include "stringref.h"

#include "pair.h"

template <class LIST>
static int op_search(const LIST &list, const StringRef &key){
	if (key.empty())
		return 1;

	auto pair = list[key];

	print(pair);

	return 0;
}

// =====================================

template <class LIST>
static int op_iterate(const LIST &list, const StringRef &key, size_t const count = 10){
	const auto bit = key == '-' ? list.begin() : list.lowerBound(key);
	const auto eit = list.end();

	size_t c = 0;
	for(auto it = bit; it != eit; ++it){
		using hm4::print;

		print(it);

		if (++c >= count)
			break;
	}

	return 0;
}

