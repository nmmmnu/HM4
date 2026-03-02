#ifndef MY_HASHTABLE_EASY_MAP_H_
#define MY_HASHTABLE_EASY_MAP_H_

#include "easyhashtable.h"
#include "map.h"

namespace myhashtable{

	template<typename K, typename T, size_t Size, template<typename,size_t> typename Storage = ArrayStorage, typename Hash = std::hash<K> >
	struct EasyMap : EasyHashtable< Map<K, T, Size, Storage, Hash> >{

		using Controller = Map<K, T, Size, Storage, Hash>;

		template<typename... Ts>
		constexpr EasyMap(Ts &&...ts) : EasyHashtable<Controller>(std::forward<Ts>(ts)...){}
	};

} // namespace myhashtable

#endif

