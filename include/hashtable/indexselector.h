#ifndef MY_HASHTABLE_INDEX_SELECTOR_H_
#define MY_HASHTABLE_INDEX_SELECTOR_H_

#include <type_traits>
#include <string>
#include <string_view>

#include <cstddef>

namespace myhashtable{

	template<template<typename, size_t> typename Index, typename K, size_t Size>
	using IndexSelector = std::conditional_t<
		std::is_same_v<K, std::string		> ||
		std::is_same_v<K, std::string_view	> ||
		false,

		Index<K, Size>,
		std::nullptr_t
	>;

} // namespace myhashtable

#endif

