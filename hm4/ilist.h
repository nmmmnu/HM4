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

template<class List>
void print(List const &list, typename List::size_type count = config::LIST_PRINT_COUNT){
	for(auto const &p : list){
		print(p);

		if (--count == 0)
			return;
	}
}

// ==============================

namespace ilist_impl_{
	template<class List, class = void>
	struct size_estimated : std::false_type{};

	template<class List>
	struct size_estimated<List, std::void_t<typename List::estimated_size> >: std::true_type{};
} // namespace ilist_impl

// ==============================

template<class List>
auto size(List const &list, std::false_type){
	return list.size();
}

template<class List>
auto size(List const &list, std::true_type){
	return narrow<typename List::size_type>(
		std::distance(std::begin(list), std::end(list))
	);
}

template<class List>
auto size(List const &list){
	using size_estimated = ilist_impl_::size_estimated<List>;

	return size(list, size_estimated{});
}

// ==============================

template<class List>
bool empty(List const &list){
	return size(list, std::false_type{}) == 0;
}

// ==============================

template<class List>
auto insert(List &list,
		std::string_view key, std::string_view val,
		uint32_t const expires = 0, uint32_t const created = 0){

	return list.insertSmartPtrPair_(
		Pair::smart_ptr::create(list.getAllocator(), key, val, expires, created)
	);
}

template<class List>
auto insert(List &list, Pair const &src){

	return list.insertSmartPtrPair_(
		Pair::smart_ptr::clone(list.getAllocator(), src)
	);
}

// ==============================

template<class List, bool B>
auto getIterator(List const &list, std::bool_constant<B>){
	if constexpr(B)
		return std::begin(list);
	else
		return std::end(list);
}

template<class List, bool B>
auto getIterator(List const &list, std::string_view const key, std::bool_constant<B> const exact){
	if (key.empty())
		return std::begin(list);
	else
		return list.find(key, exact);
}

} // namespace

#endif

