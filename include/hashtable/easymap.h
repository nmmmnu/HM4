#ifndef MY_HASHTABLE_EASY_MAP_H_
#define MY_HASHTABLE_EASY_MAP_H_

#include "mypair.h"
#include "easyhashtable.h"
#include "arraystorage.h"

#include <functional>	// std::hash

namespace myhashtable{

	template<typename K, typename T, typename Hash = std::hash<K> >
	struct Map{
		using key_type		= K;
		using mapped_type	= T;
		using value_type	= MyPair<key_type, mapped_type>;

	public:
		constexpr static size_t hash(key_type const &key){
			return Hash{}(key);
		}

		constexpr static key_type    const &getKey(value_type const &data){
			return data.first;
		}

		constexpr static mapped_type const &getVal(value_type const &data){
			return data.second;
		}

		constexpr static mapped_type &getVal(value_type &data){
			return data.second;
		}
	};



	template<
		typename	K,
		typename	T,
		size_t		MaxItems,
		size_t		Size,
		template<typename,size_t,size_t> typename Storage = ArrayStorage,
		typename	Hash = std::hash<K>
	>
	using EasyMap = EasyHashtable<
				Map<K, T, Hash>,
				Storage<
					typename Map<K, T, Hash>::value_type,
					MaxItems,
					Size
				>
			>;

} // namespace myhashtable

#endif

