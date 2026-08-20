#ifndef MULBERRY_HASH_MIXER_H_
#define MULBERRY_HASH_MIXER_H_

#include <cstdint>

constexpr uint64_t mulberry_hash_mixer64(uint64_t x, uint64_t seed = 0) noexcept{
	uint64_t z = (x ^ seed) + 0x9E3779B97F4A7C15ULL;
	z = (z ^ (z >> 30)) * 0xBF58476D'1CE4E5B9;
	z = (z ^ (z >> 27)) * 0x94D049BB'133111EB;
	return z ^ (z >> 31);
}

constexpr uint64_t mulberry_hash_mixer64_nz(uint64_t x, uint64_t seed = 0) noexcept{
	constexpr uint64_t zero		= 0xDEED'BEEF'ABBA'B0BA;
	constexpr uint64_t zeroResult	= mulberry_hash_mixer64(zero);

	static_assert(zeroResult > 0);

	uint64_t const result = mulberry_hash_mixer64( x, seed );

	return result ? result : zeroResult;
}

struct MulberryHashMixer64{
	constexpr MulberryHashMixer64(uint64_t seed = 0) : seed(seed){}

	constexpr uint64_t operator()(){
		return mulberry_hash_mixer64_nz(id++, seed);
	}

	constexpr void reset(){
		id = 0;
	}

private:
	uint64_t id = 0;
	uint64_t seed;
};

// ----------------------------------

constexpr uint32_t mulberry_hash_mixer32(uint32_t x, uint32_t seed = 0) noexcept{
	uint32_t z = (x ^ seed) + 0x6D2B'79F5;
	z = (z ^ (z >> 15)) * (z | 1U);
	z ^= z + (z ^ (z >> 7)) * (z | 61u);
	return z ^ (z >> 14);
}

constexpr uint32_t mulberry_hash_mixer32_nz(uint32_t x, uint32_t seed = 0) noexcept{
	constexpr uint32_t zero		= 0xDEED'BEEF;
	constexpr uint32_t zeroResult	= mulberry_hash_mixer32(zero);

	static_assert(zeroResult > 0);

	uint32_t const result = mulberry_hash_mixer32( x, seed );

	return result ? result : zeroResult;
}

struct MulberryHashMixer32{
	constexpr MulberryHashMixer32(uint32_t seed = 0) : seed(seed){}

	constexpr uint32_t operator()(){
		return mulberry_hash_mixer32_nz(id++, seed);
	}

	constexpr void reset(){
		id = 0;
	}

private:
	uint32_t id = 0;
	uint32_t seed;
};

#endif

