#ifndef MY_HASHTABLE_EASY_SET_H_
#define MY_HASHTABLE_EASY_SET_H_

#include "easyhashtable.h"
#include "arraystorage.h"

#include <functional>	// std::hash

namespace myhashtable{

	template<typename K, typename Hash = std::hash<K> >
	struct Set{
		using key_type		= K;
		using mapped_type	= key_type;
		using value_type	= key_type;

	public:
		constexpr static size_t hash(key_type const &key){
			return Hash{}(key);
		}

		constexpr static key_type    const &getKey(key_type const &data){
			return data;
		}

		// there is no non-const version, because hashtable can be ruined
		constexpr static mapped_type const &getVal(key_type const &data){
			return data;
		}

	};



	template<
		typename	K,
		size_t		MaxItems,
		size_t		Size,
		template<typename,size_t,size_t> typename Storage = ArrayStorage,
		typename	Hash = std::hash<K>
	>
	using EasySet = EasyHashtable<
				Set<K, Hash>,
				Storage<
					typename Set<K, Hash>::value_type,
					MaxItems,
					Size
				>
			>;

} // namespace myhashtable

#endif

