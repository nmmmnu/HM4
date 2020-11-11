#ifndef STRING_HASH_H_
#define STRING_HASH_H_

#include "myendian.h"

#include <cstring>
#include <cstdio>

#include <string_view>
#include <algorithm>
#include <utility>
#include <type_traits>
#include <iostream>

template<typename T>
class StringHash{
public:
	constexpr static auto N = sizeof(T);

private:
	enum class CreateType{
		CSTR,
		SIZE,
		ALIGNED
	};

	template<bool BigEndian, CreateType CT>
	static T create__(const char *src, size_t size = 0) noexcept{
		union{
			char	s[N];
			T	u = 0;
		};

		/* switch constexpr */ {
			if constexpr(CT == CreateType::CSTR)
				strncpy(s, src, N);

			if constexpr(CT == CreateType::SIZE)
				memcpy(s, src, std::min(N, size));

			if constexpr(CT == CreateType::ALIGNED){
				// string is BE, so mask must be BE as well.
				auto const mask_ = ~ T{ 0 } >> (size * 8);	// FFFFFF -> 00FFFF
			//	std::cout << mask_ << '\n';
				auto const mask  = betoh( ~(mask_) );	// 00FFFF -> FF0000

				printf("%16x\n", mask_);
				printf("%16x\n", ~mask_);
				printf("%16x\n", ~0u);

				u = *reinterpret_cast<const T *>(src) & mask;

				printf("%s | %zu | %8x | %8x | %zu \n", src, size, mask, u, N );
			}
		}

		if constexpr(BigEndian == false)
			return betoh(u);
		else
			return u;
	}

	template<class Result>
	static std::pair<bool, Result> sizeCheck__(T const a, T const b, Result const result) noexcept{
		constexpr auto MASK = htobe( T{ 0xFF } << (sizeof(T) - 1) * 8 );

		bool const size_overflow = (a | b) & MASK;

		return { !size_overflow, result };
	}

public:
	template<bool BigEndian = false>
	static T create(const char *src) noexcept{
		return create__<BigEndian, CreateType::CSTR>(src);
	}

	template<bool BigEndian = false>
	static T create(const char *src, size_t const size) noexcept{
		return create__<BigEndian, CreateType::SIZE>(src, size);
	}

	template<bool BigEndian = false>
	static T create(std::string_view const s) noexcept{
		return create<BigEndian>(s.data(), s.size());
	}

	template<bool BigEndian = false>
	static T createAligned(const char *src, size_t const size) noexcept{
		return create__<BigEndian, CreateType::ALIGNED>(src, size);
	}

	static std::pair<bool, int> compare(T const a, T const b) noexcept{
		auto comp = [](auto a, auto b){
			return (a > b) - (a < b);
		};

		if (int const x = comp(a, b); x)
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

