#ifndef PAIR_H
#define PAIR_H

#include <cstdint>
#include <cstddef>

namespace hm4{
	namespace PairConf{
		constexpr size_t	ALIGN		= sizeof(uint64_t);
		constexpr size_t	HLINE_SIZE	= sizeof(uint64_t);

		constexpr const char	*EMPTY_MESSAGE	= "---empty-pair---";
	}
}

#include "pair_owner.h"

#endif

