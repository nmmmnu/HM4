#ifndef MURMUR_HASH_MIXER_H_
#define MURMUR_HASH_MIXER_H_

#include <cstdint>

constexpr uint64_t murmur_hash_mixer64(uint64_t x, uint64_t seed = 0){
	x ^= seed;
	x ^= x >> 33;
	x *= 0xff51afd7'ed558ccd;
	x ^= x >> 33;
	x *= 0xc4ceb9fe'1a85ec53;
	x ^= x >> 33;
	return x;
}

constexpr uint64_t murmur_hash_mixer64_nz(uint64_t x, uint64_t seed = 0){
	// murmur_hash_mixer64(0x12345678'90abcdef, 0x12345678'90abcdef)
	// will return 0

	constexpr uint64_t zero		= 0xDEED'BEEF'ABBA'B0BA;
	constexpr uint64_t zeroResult	= murmur_hash_mixer64(zero);

	static_assert(zeroResult > 0);

	uint64_t const result = murmur_hash_mixer64( x, seed );

	return result ? result : zeroResult;
}

#endif

