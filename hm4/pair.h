#ifndef PAIR_H
#define PAIR_H

#include <cstdint>

namespace hm4{
	namespace PairConf{
		constexpr std::size_t	ALIGN		= sizeof(uint64_t);
		constexpr std::size_t	HLINE_SIZE	= sizeof(uint64_t);
	}
}

#include "pair_owner.h"

#endif

