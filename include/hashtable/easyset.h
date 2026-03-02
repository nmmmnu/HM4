#ifndef MY_HASHTABLE_EASY_SET_H_
#define MY_HASHTABLE_EASY_SET_H_

#include "easyhashtable.h"
#include "set.h"

namespace myhashtable{

	template<typename K, size_t MaxItems, size_t Size, template<typename,size_t,size_t> typename Storage = ArrayStorage, typename Hash = std::hash<K> >
	struct EasySet : EasyHashtable< Set<K, MaxItems, Size, Storage, Hash> >{

		using Controller = Set<K, MaxItems, Size, Storage, Hash>;

		template<typename... Ts>
		constexpr EasySet(Ts &&...ts) : EasyHashtable<Controller>(std::forward<Ts>(ts)...){}

		// there is no non-const version, because hashtable can be ruined
		typename Controller::mapped_type *find(typename Controller::key_type const &key) = delete;
	};

} // namespace myhashtable

#endif

