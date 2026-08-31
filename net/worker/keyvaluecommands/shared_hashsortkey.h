#ifndef SHARED_HASH_SORT_KEY
#define SHARED_HASH_SORT_KEY

#include "hexconvert.h"
#include <array>

namespace net::worker::shared::sortkey{

	constexpr size_t keySortSize		= 16; // size of uint64 as hex



	template<size_t N>
	std::string_view makeHashKeySort(std::string_view keySub, std::array<char, N> &buffer){
		static_assert(N > keySortSize);

		return hex_convert::toHex(murmur_hash64a(keySub), buffer);
	}

}

#endif

