#ifndef MURMUR_HASH_MIXER_H_
#define MURMUR_HASH_MIXER_H_

#include <cstdint>

constexpr uint64_t murmur_hash_mixer64(uint64_t x, uint64_t seed = 0) noexcept{
	x ^= seed;
	x ^= x >> 33;
	x *= 0xff51afd7'ed558ccd;
	x ^= x >> 33;
	x *= 0xc4ceb9fe'1a85ec53;
	x ^= x >> 33;
	return x;
}

constexpr uint64_t murmur_hash_mixer64_nz(uint64_t x, uint64_t seed = 0) noexcept{
	constexpr uint64_t zero		= 0xDEED'BEEF'ABBA'B0BA;
	constexpr uint64_t zeroResult	= murmur_hash_mixer64(zero);

	static_assert(zeroResult > 0);

	uint64_t const result = murmur_hash_mixer64( x, seed );

	return result ? result : zeroResult;
}

struct MurmurHashMixer64{
	constexpr MurmurHashMixer64(uint64_t seed = 0) : seed(seed){}

	constexpr uint64_t operator()(){
		return murmur_hash_mixer64_nz(id++, seed);
	}

	constexpr void reset(){
		id = 0;
	}

private:
	uint64_t id = 0;
	uint64_t seed;
};

// ----------------------------------

constexpr uint32_t murmur_hash_mixer32(uint32_t x, uint32_t seed = 0) noexcept{
	x ^= seed;
	x ^= x >> 16;
	x *= 0x85eb'ca6b;
	x ^= x >> 13;
	x *= 0xc2b2'ae35;
	x ^= x >> 16;
	return x;
}

constexpr uint32_t murmur_hash_mixer32_nz(uint32_t x, uint32_t seed = 0) noexcept{
	constexpr uint32_t zero		= 0xDEED'BEEF;
	constexpr uint32_t zeroResult	= murmur_hash_mixer32(zero);

	static_assert(zeroResult > 0);

	uint32_t const result = murmur_hash_mixer32( x, seed );

	return result ? result : zeroResult;
}

struct MurmurHashMixer32{
	constexpr MurmurHashMixer32(uint32_t seed = 0) : seed(seed){}

	constexpr uint32_t operator()(){
		return murmur_hash_mixer32_nz(id++, seed);
	}

	constexpr void reset(){
		id = 0;
	}

private:
	uint32_t id = 0;
	uint32_t seed;
};

#endif

