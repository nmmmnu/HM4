#ifndef MURMUR_HASH_MIXER_H_
#define MURMUR_HASH_MIXER_H_

#include <cstdint>

constexpr uint64_t murmur_hash_mixer64(uint64_t x){
	x ^= x >> 33;
	x *= 0xff51afd7'ed558ccd;
	x ^= x >> 33;
	x *= 0xc4ceb9fe'1a85ec53;
	x ^= x >> 33;
	return x;
}

constexpr uint64_t murmur_hash_mixer64_nz(uint64_t x){
	uint64_t const zero = 0xDEED'BEEF'ABBA'B0BA;
	return murmur_hash_mixer64( x ? x : zero );
}

#endif

