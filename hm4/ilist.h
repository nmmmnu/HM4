#ifndef _MY_LIST_H
#define _MY_LIST_H

#include <cstdint>

#include "pair.h"

namespace hm4{

namespace config{
	using size_type		= uint64_t;
	using difference_type	= int64_t;

	constexpr size_type	LIST_PRINT_COUNT	= 10;
}

// ==============================

template<class LIST>
void print(LIST const &list, typename LIST::size_type count = config::LIST_PRINT_COUNT){
	for(auto const &p : list){
		print(p);

		if (--count == 0)
			return;
	}
}

template<class LIST>
auto size(LIST const &list){
	return list.size();
}

template<class LIST>
auto empty(LIST const &list){
	return list.size() == 0;
}


} // namespace

#endif

