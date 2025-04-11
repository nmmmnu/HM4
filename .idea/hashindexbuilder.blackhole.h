#include "hashindexbuilder.misc.h"

#include "filewriter.h"

#include <cstring>	// memset

namespace hm4::disk::hash::algo{

	struct HashIndexBlackHoleBuilder{
		constexpr bool operator()(Pair const &) const{
			return false;
		}
	};

} // namespace hm4::disk::hash::algo

