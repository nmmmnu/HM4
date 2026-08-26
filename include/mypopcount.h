#ifndef POPCOUNT_H_
#define POPCOUNT_H_

#include <cstdint>
#include <cassert>

#include <string_view>

#include "forcevectorize.h"

namespace MyPopcount{

	size_t popcount(std::string_view a, size_t bits){
		{
			[[maybe_unused]]
			const size_t bytesRequired = (bits + 7) / 8;

			assert(a.size() >= bytesRequired	&& "Views must contain at least enough bytes for bits");
		}

		size_t result = 0;

		size_t const size64 = bits / (sizeof(uint64_t) * 8);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());

		FORCE_VECTORIZE
		for (size_t i = 0; i < size64; ++i)
			result += static_cast<size_t>(__builtin_popcountll(pa64[i]));

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t  *>(pa64 + size64);

		size_t bitsRemaining = bits % (sizeof(uint64_t) * 8);
		size_t size8         = bitsRemaining / 8;

		FORCE_VECTORIZE
		for (size_t i = 0; i < size8; ++i)
			result += static_cast<size_t>(__builtin_popcount(pa8[i]));

		if (const size_t bitsTail = bitsRemaining % 8; bitsTail){
			uint8_t const mask = static_cast<uint8_t>((1u << bitsTail) - 1u);

			result += static_cast<size_t>(__builtin_popcount( (pa8[size8]) & mask ));
		}

		return result;
	}



	template<typename F>
	size_t popcount2F(std::string_view a, std::string_view b, size_t bits, F f){
		{
			[[maybe_unused]]
			const size_t bytesRequired = (bits + 7) / 8;

			assert(a.size() == b.size()		&& "Size of the views must be the same");
			assert(a.size() >= bytesRequired	&& "Views must contain at least enough bytes for bits");
		}

		size_t result = 0;

		size_t const size64 = bits / (sizeof(uint64_t) * 8);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());
		const uint64_t *pb64 = reinterpret_cast<const uint64_t *>(b.data());

		FORCE_VECTORIZE
		for (size_t i = 0; i < size64; ++i)
			result += static_cast<size_t>(__builtin_popcountll( f(pa64[i], pb64[i]) ));

		const uint8_t  *pa8  = reinterpret_cast<const uint8_t  *>(pa64 + size64);
		const uint8_t  *pb8  = reinterpret_cast<const uint8_t  *>(pb64 + size64);

		size_t bitsRemaining = bits % (sizeof(uint64_t) * 8);
		size_t size8         = bitsRemaining / 8;

		FORCE_VECTORIZE
		for (size_t i = 0; i < size8; ++i)
			result += static_cast<size_t>(__builtin_popcount( f(pa8[i], pb8[i]) ));

		if (const size_t bitsTail = bitsRemaining % 8; bitsTail){
			uint8_t const mask = static_cast<uint8_t>((1u << bitsTail) - 1u);

			result += static_cast<size_t>(__builtin_popcount( f(pa8[size8], pb8[size8]) & mask ));
		}

		return result;
	}

	size_t popcountXOR(std::string_view a, std::string_view b, size_t bits){
		auto f = [](auto a, auto b){
			static_assert(std::is_same_v<decltype(a), decltype(b)>);

			return static_cast<decltype(a)>(a ^ b);
		};

		return popcount2F(a, b, bits, f);
	}

	size_t popcountAND(std::string_view a, std::string_view b, size_t bits){
		auto f = [](auto a, auto b){
			static_assert(std::is_same_v<decltype(a), decltype(b)>);

			return static_cast<decltype(a)>(a & b);
		};

		return popcount2F(a, b, bits, f);
	}



	auto popcountDOT(std::string_view a, std::string_view b, size_t bits){
		{
			[[maybe_unused]]
			const size_t bytesRequired = (bits + 7) / 8;

			assert(a.size() == b.size()		&& "Size of the views must be the same");
			assert(a.size() >= bytesRequired	&& "Views must contain at least enough bytes for bits");
		}

		struct Result{
		    size_t dot   = 0;
		    size_t normA = 0;
		    size_t normB = 0;
		};

		Result result;

		size_t const size64 = bits / (sizeof(uint64_t) * 8);

		const uint64_t *pa64 = reinterpret_cast<const uint64_t *>(a.data());
		const uint64_t *pb64 = reinterpret_cast<const uint64_t *>(b.data());

		FORCE_VECTORIZE
		for (size_t i = 0; i < size64; ++i){
			const auto byteA = pa64[i];
			const auto byteB = pb64[i];

			result.dot   += static_cast<size_t>(__builtin_popcountll(byteA & byteB));
			result.normA += static_cast<size_t>(__builtin_popcountll(byteA));
			result.normB += static_cast<size_t>(__builtin_popcountll(byteB));
		}

		const uint8_t *pa8 = reinterpret_cast<const uint8_t *>(pa64 + size64);
		const uint8_t *pb8 = reinterpret_cast<const uint8_t *>(pb64 + size64);

		const size_t bitsRemaining = bits % (sizeof(uint64_t) * 8);
		const size_t size8 = bitsRemaining / 8;

		FORCE_VECTORIZE
		for (size_t i = 0; i < size8; ++i){
			const auto byteA = pa8[i];
			const auto byteB = pb8[i];

			result.dot   += static_cast<size_t>(__builtin_popcount(byteA & byteB));
			result.normA += static_cast<size_t>(__builtin_popcount(byteA));
			result.normB += static_cast<size_t>(__builtin_popcount(byteB));
		}

		// 3. Остатъчни битове (1 до 7) с маскиране
		if (const size_t bitsTail = bitsRemaining % 8; bitsTail){
			const uint8_t mask  = static_cast<uint8_t>((1u << bitsTail) - 1u);

			const uint8_t byteA = pa8[size8] & mask;
			const uint8_t byteB = pb8[size8] & mask;

			result.dot   += static_cast<size_t>(__builtin_popcount(byteA & byteB));
			result.normA += static_cast<size_t>(__builtin_popcount(byteA));
			result.normB += static_cast<size_t>(__builtin_popcount(byteB));
		}

		return result;
	}

} //namespace MyPopcount

#endif

