#ifndef _PAIR_CONF_H
#define _PAIR_CONF_H

#include <cstdint>

namespace hm4{

namespace PairConf{
	constexpr uint16_t	MAX_KEY_SIZE	=      1024;	// MySQL is 1000
	constexpr uint32_t	MAX_VAL_SIZE	= 16 * 1024;
}

}

#endif

