#ifndef SOFTWARE_PREFETCH_H_
#define SOFTWARE_PREFETCH_H_

namespace builtin_prefetch_config{
		constexpr bool USE_PREFETCH = true;
}

	#ifndef __clang__

		constexpr void builtin_prefetch(const void *addr){
			if constexpr(builtin_prefetch_config::USE_PREFETCH){
				__builtin_prefetch(addr, 0, 1);
			}
		}

	#else

		inline void builtin_prefetch(const void *addr){
			if constexpr(builtin_prefetch_config::USE_PREFETCH){
				__builtin_prefetch(addr, 0, 1);
			}
		}

	#endif

#endif

