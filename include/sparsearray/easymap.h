#ifndef MY_SPARSE_ARRAY_EASYMAP_
#define MY_SPARSE_ARRAY_EASYMAP_

#include "sparsearray.h"
#include "mypair.h"

namespace mysparsearray{

	namespace impl_{

		template<typename K, typename T>
		struct SparseMapController{
			static_assert(
				std::is_same_v<K, uint8_t>  ||
				std::is_same_v<K, uint16_t> ||
				std::is_same_v<K, uint32_t> ||
				std::is_same_v<K, uint64_t>
			);

			using key_type		= K;
			using value_type	= T;
			using mapped_type	= MyPair<key_type, value_type>;

			[[nodiscard]]
			static constexpr key_type const &getKey(mapped_type const &value){
				return value.first;
			}

			[[nodiscard]]
			static constexpr value_type const &getVal(mapped_type const &value){
				return value.second;
			}

			[[nodiscard]]
			static constexpr value_type       &getVal(mapped_type       &value){
				return value.second;
			}
		};

	} // namespace impl_

	template<typename K, typename V, template<typename> typename Vector = std::vector>
	using EasyMap = SparseArray<K, impl_::SparseMapController<K, V>, Vector>;

} // namespace mysparsemap

#endif

