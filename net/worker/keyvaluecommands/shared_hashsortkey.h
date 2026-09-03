#ifndef SHARED_HASH_SORT_KEY
#define SHARED_HASH_SORT_KEY

#include "hexconvert.h"
#include <array>

namespace net::worker::shared::sortkey{

	constexpr size_t keySortSize(){
		return 16; // size of uint64 as hex
	}

	constexpr size_t keySortSize(std::string_view keySort){
		if (keySort.empty())
			return keySortSize();

		return keySort.size();
	}

	template<size_t N>
	std::string_view makeHashKeySort(std::string_view keySub, std::string_view keySort, std::array<char, N> &buffer){
		static_assert(N > keySortSize());

		if (keySort.empty())
			return hex_convert::toHex(murmur_hash64a(keySub), buffer);

		return keySort;
	}

}

#endif

