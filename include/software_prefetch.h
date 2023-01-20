#ifndef SOFTWARE_PREFETCH_H_
#define SOFTWARE_PREFETCH_H_

namespace builtin_prefetch_config{
	constexpr bool USE_PREFETCH = true;
}

template<typename ...Ts>
constexpr void builtin_prefetch(const void *addr, Ts ...ts){
	if constexpr(builtin_prefetch_config::USE_PREFETCH){
		__builtin_prefetch(addr, std::forward<Ts>(ts)...);
	}
}

#endif

