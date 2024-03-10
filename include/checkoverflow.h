#ifndef CHECK_OVERFLOW_
#define CHECK_OVERFLOW_

#include <limits>

namespace safe_overflow{

	template<typename T>
	[[nodiscard]]
	constexpr T incr(T a, T x){
		constexpr auto MIN = std::numeric_limits<T>::min();
		constexpr auto MAX = std::numeric_limits<T>::max();

		if (x > 0 && a > MAX - x){
			// overflow
			return MAX;
		}

		if (x < 0 && a < MIN - x){
			// underflow
			return MIN;
		}

		return a + x;
	}

	template<typename T>
	[[nodiscard]]
	constexpr T decr(T a, T x){
		constexpr auto MIN = std::numeric_limits<T>::min();
		constexpr auto MAX = std::numeric_limits<T>::max();

		if (x < 0 && a > MAX + x){
			// overflow
			return MAX;
		}

		if (x > 0 && a < MIN + x){
			// underflow
			return MIN;
		}

		return a - x;
	}

	namespace test_{
		namespace t1{
			using T = unsigned char;
			static_assert(incr<T>(150,  150) == std::numeric_limits<T>::max());
			static_assert(incr<T>(100,  100) == 200);

			static_assert(decr<T>(100,  150) == std::numeric_limits<T>::min());
			static_assert(decr<T>(100,   50) == 50);

		}

		namespace t2{
			using T = signed char;
			static_assert(incr<T>(-50, -120) == std::numeric_limits<T>::min());
			static_assert(incr<T>(100,  100) == std::numeric_limits<T>::max());
			static_assert(incr<T>( 50,   50) == 100);

			static_assert(decr<T>(-50,  120) == std::numeric_limits<T>::min());
			static_assert(decr<T>(100, -100) == std::numeric_limits<T>::max());
			static_assert(decr<T>(100,   50) == 50);
		}
	}

}// namespace safe_overflow

#endif

