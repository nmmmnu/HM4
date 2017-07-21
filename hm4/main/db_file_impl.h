#include "stringref.h"

template <class LIST>
static int op_search(const LIST &list, const StringRef &key){
	if (key.empty())
		return 1;

	const auto &pair = list[key];

	pair.print();

	return 0;
}

// =====================================

template <class LIST>
static int op_iterate(const LIST &list, const StringRef &key, size_t const count = 10){
	const auto bit = key == '-' ? list.begin() : list.lowerBound(key);
	const auto eit = list.end();

	size_t c = 0;
	for(auto it = bit; it != eit; ++it){
		it->print();

		if (++c >= count)
			break;
	}

	return 0;
}

