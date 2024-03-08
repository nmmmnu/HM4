#ifndef SHARED_INCR_H_
#define SHARED_INCR_H_

namespace net::worker::shared::incr{

	template<typename T>
	T incr(T &a, uint64_t increment){
		constexpr auto MAX    = std::numeric_limits<T>::max();
		constexpr auto MAX_BE = htobe(MAX);

		auto const x = betoh<T>(a);

		if (increment > MAX || x > MAX - increment){
			// return max
			a = MAX_BE;

			return MAX;
		}else{
			// cast is safe now
			T const c = x + static_cast<T>(increment);

			a = htobe<T>(c);

			return c;
		}
	}

}

#endif

