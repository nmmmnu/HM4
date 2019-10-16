#ifndef MY_LIST_H_
#define MY_LIST_H_

#include <cstdint>
#include <iterator>	// std::distance

#include "pair.h"

#include "mynarrow.h"

namespace hm4{

namespace config{
	using size_type		= size_t;
	using difference_type	= ptrdiff_t;

	static_assert(sizeof(size_type      ) == 8, "You need 64bit system!");
	static_assert(sizeof(difference_type) == 8, "You need 64bit system!");

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

// ==============================

namespace ilist_impl_{
	template<class LIST, class = void>
	struct size_estimated : std::false_type{};

	template<class LIST>
	struct size_estimated<LIST, std::void_t<typename LIST::estimated_size> >: std::true_type{};
} // namespace ilist_impl

// ==============================

template<class LIST>
auto size(LIST const &list, std::false_type){
	return list.size();
}

template<class LIST>
auto size(LIST const &list, std::true_type){
	return narrow<typename LIST::size_type>(
		std::distance(std::begin(list), std::end(list))
	);
}

template<class LIST>
auto size(LIST const &list){
	using size_estimated = ilist_impl_::size_estimated<LIST>;

	return size(list, size_estimated{});
}

// ==============================

template<class LIST>
bool empty(LIST const &list){
	return size(list, std::false_type{}) == 0;
}

// ==============================

template<class List>
auto getIterator(List const &list, std::true_type){
	return std::begin(list);
}

template<class List>
auto getIterator(List const &list, std::false_type){
	return std::end(list);
}

template<class List, bool B>
auto getIterator(List const &list, std::string_view const key, std::bool_constant<B> const exact){
	return list.find(key, exact);
}



} // namespace

#endif

