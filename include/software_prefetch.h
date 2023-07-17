#ifndef SOFTWARE_PREFETCH_H_
#define SOFTWARE_PREFETCH_H_

namespace builtin_prefetch_config{
		constexpr bool USE_PREFETCH = true;
}


	#ifndef __clang__

		template<typename ...Ts>
		constexpr void builtin_prefetch(const void *addr, Ts ...ts){
			if constexpr(builtin_prefetch_config::USE_PREFETCH){
				__builtin_prefetch(addr, std::forward<Ts>(ts)...);
			}
		}

	#else
		// clang have problem with forwarding parameters to __builtin_prefetch
		constexpr void builtin_prefetch(const void *, ...){
		}

	#endif

#endif

