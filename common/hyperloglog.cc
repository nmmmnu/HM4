#include "hyperloglog.h"

#include "murmur_hash_64a.h"

#include <cmath>

namespace{
	uint32_t getLeadingBits(uint64_t const hash, uint8_t bits){
		return uint32_t( hash >> (64 - bits));
	}

	uint8_t countLeadingZeros(uint64_t const hash, uint8_t bits){
		uint64_t const mask = (1 << bits) - 1;
		uint64_t const x = (hash << bits) | mask;

		auto const result = __builtin_clzll(x);

	//	assert(result < 0xff);

		return static_cast<uint8_t>(result);
	}

	uint32_t countZeros(const uint8_t *data, uint32_t size) {
		uint32_t zeros = 0;

		for(size_t i = 0; i < size; ++i)
			if (data[i] == 0)
				++zeros;

		return zeros;
	}

	constexpr static double calcAlpha(uint32_t m){
		switch (m) {
		case 16:	return 0.673;
		case 32:	return 0.697;
		case 64:	return 0.709;
		default:	return 0.7213 / (1.0 + 1.079 / m);
		}
	}
}

namespace hyperloglog::hyperloglog_implementation{

	CalcAddResult calcAdd(const char *s, size_t size, uint8_t bits){
		uint64_t const hash = murmur_hash64a(s, size);

		uint32_t const index = getLeadingBits   (hash, bits);
		uint8_t  const rank  = countLeadingZeros(hash, bits) + 1;

		return { index, rank };
	}

	double estimate(const uint8_t *data, uint32_t size){
		double sum = 0.0;
		for(size_t i = 0; i < size; ++i)
			sum += 1.0 / (1 << data[i]);

		uint32_t const m = size;

		double estimate = calcAlpha(m) * m * m / sum;

		 // small range correction
		if (estimate <= 2.5 * m){
			uint32_t zeros = countZeros(data, size);

			if (zeros == 0){
				// if no empty registers, use the estimate we already have
				return estimate;
			}else{
				// balls and bins correction
				return m * std::log(static_cast<double>(m)/ zeros);
			}
		}

		static constexpr double pow_2_32 = uint64_t(2) << 31;

		// large range correction
		if (estimate > pow_2_32 / 30){
			return ( - pow_2_32 ) * log(1.0 - (estimate / pow_2_32));
		}

		return estimate;
	}

} // namespace hyperloglog::hyperloglog_implementation


