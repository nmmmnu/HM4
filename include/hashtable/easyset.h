#ifndef MY_HASHTABLE_EASY_SET_H_
#define MY_HASHTABLE_EASY_SET_H_

#include "easyhashtable.h"
#include "arraystorage.h"
#include "tophashindex.h"
#include "indexselector.h"

namespace myhashtable{

	namespace myhashtable_impl_{

		template<typename K>
		struct SetAdapter{
			using key_type		= K;
			using mapped_type	= key_type;
			using value_type	= key_type;

		public:
			[[nodiscard]]
			constexpr static key_type    const &getKey(key_type const &data){
				return data;
			}

			// there is no non-const version, because hashtable can be ruined
			[[nodiscard]]
			constexpr static mapped_type const &getVal(key_type const &data){
				return data;
			}

		};

	} // namespace myhashtable_impl_



	template<
		typename	K,
		size_t		MaxItems,
		size_t		Size,
		template<typename,size_t,size_t> typename Storage = ArrayStorage,
		typename	Hash = std::hash<K>
	>
	using EasySet = EasyHashtable<
				myhashtable_impl_::SetAdapter<K>,
				IndexSelector<TopHashIndex, K, Size>,
				Storage<
					typename myhashtable_impl_::SetAdapter<K>::value_type,
					MaxItems,
					Size
				>,
				Hash
			>;

} // namespace myhashtable

#endif

