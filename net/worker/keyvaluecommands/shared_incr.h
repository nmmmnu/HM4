#ifndef SHARED_INCR_H_
#define SHARED_INCR_H_

#include "checkoverflow.h"

namespace net::worker::shared::incr{

	template<bool Sign, typename T>
	constexpr T incr(T &val, uint64_t inc){
		T a = betoh<T>(val);
		T const x = static_cast<T>(inc);

		if constexpr(Sign)
			a = safe_overflow::incr(a, x);
		else
			a = safe_overflow::decr(a, x);

		val = htobe<T>(a);

		return a;
	}

}

#endif

