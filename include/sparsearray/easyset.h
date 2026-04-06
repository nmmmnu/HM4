#ifndef MY_SPARSE_ARRAY_EASYSET_
#define MY_SPARSE_ARRAY_EASYSET_

#include "sparsearray.h"

namespace mysparsearray{

	namespace impl_{

		template<typename K>
		struct SparseSetController{
			static_assert(
				std::is_same_v<K, uint8_t>  ||
				std::is_same_v<K, uint16_t> ||
				std::is_same_v<K, uint32_t> ||
				std::is_same_v<K, uint64_t>
			);

			using key_type		= K;
			using value_type	= K;
			using mapped_type	= K;

			[[nodiscard]]
			static constexpr key_type const &getKey(mapped_type const &value){
				return value;
			}

			// there is no non-const version, because hashtable can be ruined
			[[nodiscard]]
			static constexpr value_type const &getVal(mapped_type const &value){
				return value;
			}
		};

	} // namespace impl_

	template<typename K, template<typename> typename Vector = std::vector>
	using EasySet = SparseArray<K, impl_::SparseSetController<K>, Vector>;

} // namespace mysparsemap

#endif

