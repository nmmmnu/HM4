#ifndef STRING_HASH_H_
#define STRING_HASH_H_

#include "myendian.h"

#include <cstring>

#include <string_view>
#include <algorithm>
#include <utility>
#include <type_traits>

template<typename T>
class StringHash{
public:
	constexpr static auto N = sizeof(T);

private:
	template<bool KnownSize>
	static T createBE__(const char *src, size_t size = 0) noexcept{
		union{
			char	s[N];
			T	u = 0;
		};

		if constexpr(KnownSize)
			memcpy(s, src, std::min(N, size));
		else
			strncpy(s, src, N);

		return u;
	}

	template<bool KnownSize>
	static T create__(const char *src, size_t size = 0) noexcept{
		return betoh( createBE__<KnownSize>(src, size) );
	}

	static int compare__(T const a, T const b) noexcept{
		return (a > b) - (a < b);
	}

	template<class Result>
	static std::pair<bool, Result> sizeCheck__(T const a, T const b, Result const result) noexcept{
		constexpr auto MASK = htobe( T{ 0xFF } << (sizeof(T) - 1) * 8 );

		bool const size_overflow = (a | b) & MASK;

		return { !size_overflow, result };
	}

public:
	static T create(const char *src) noexcept{
		return create__<false>(src);
	}

	static T create(const char *src, size_t const size) noexcept{
		return create__<true>(src, size);
	}

	static T create(std::string_view const s) noexcept{
		return create(s.data(), s.size());
	}

	static T createBE(const char *src) noexcept{
		return createBE__<false>(src);
	}

	static T createBE(const char *src, size_t const size) noexcept{
		return createBE__<true>(src, size);
	}

	static T createBE(std::string_view const s) noexcept{
		return createBE(s.data(), s.size());
	}

	static std::pair<bool, int> compare(T const a, T const b) noexcept{
		if (int const x = compare__(a, b); x)
			return { true, x };

		return sizeCheck__(a, b, 0);
	}

	static std::pair<bool, bool> equals(T const a, T const b) noexcept{
		if (a != b)
			return { true, false };

		return sizeCheck__(a, b, true);
	}
};


#endif

