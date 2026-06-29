#ifndef MY_HASHTABLE_EASY_MAP_H_
#define MY_HASHTABLE_EASY_MAP_H_

#include "mypair.h"
#include "easyhashtable.h"
#include "arraystorage.h"
#include "tophashindex.h"
#include "indexselector.h"

namespace myhashtable{

	namespace myhashtable_impl_{

		template<typename K, typename T>
		struct MapAdapter{
			using key_type		= K;
			using mapped_type	= T;
			using value_type	= MyPair<key_type, mapped_type>;

		public:
			[[nodiscard]]
			constexpr static key_type    const &getKey(value_type const &data){
				return data.first;
			}

			[[nodiscard]]
			constexpr static mapped_type const &getVal(value_type const &data){
				return data.second;
			}

			[[nodiscard]]
			constexpr static mapped_type       &getVal(value_type &data){
				return data.second;
			}
		};

	} // namespace myhashtable_impl_



	template<
		typename	K,
		typename	T,
		size_t		MaxItems,
		size_t		Size,
		template<typename,size_t,size_t> typename Storage = ArrayStorage,
		typename	Hash = std::hash<K>
	>
	using EasyMap = EasyHashtable<
				myhashtable_impl_::MapAdapter<K, T>,
				IndexSelector<TopHashIndex, K, Size>,
				Storage<
					typename myhashtable_impl_::MapAdapter<K, T>::value_type,
					MaxItems,
					Size
				>,
				Hash
			>;

} // namespace myhashtable

#endif

