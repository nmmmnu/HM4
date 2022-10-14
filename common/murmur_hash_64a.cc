#include "murmur_hash_64a.h"

// Based on
// https://github.com/hhrhhr/MurmurHash-for-Lua
// https://github.com/hhrhhr/MurmurHash-for-Lua/blob/master/MurmurHash64A.c

uint64_t murmur_hash64a(const void *key, size_t size, uint64_t seed){
	const uint64_t m = 0xc6a4a7935bd1e995LLU;
	const int r = 47;

	uint64_t h = seed ^ (size * m);

	const uint64_t *data = (const uint64_t *)key;
	const uint64_t *end = (size >> 3) + data;

	while(data != end){
		uint64_t k = *data++;

		k *= m;
		k ^= k >> r;
		k *= m;

		h ^= k;
		h *= m;
	}

	const uint8_t *data2 = (const uint8_t *)data;

	switch(size & 7){
		case 7: h ^= (uint64_t)(data2[6]) << (6 * 8);	[[fallthrough]];
		case 6: h ^= (uint64_t)(data2[5]) << (5 * 8);	[[fallthrough]];
		case 5: h ^= (uint64_t)(data2[4]) << (4 * 8);	[[fallthrough]];
		case 4: h ^= (uint64_t)(data2[3]) << (3 * 8);	[[fallthrough]];
		case 3: h ^= (uint64_t)(data2[2]) << (2 * 8);	[[fallthrough]];
		case 2: h ^= (uint64_t)(data2[1]) << (1 * 8);	[[fallthrough]];
		case 1: h ^= (uint64_t)(data2[0]) << (0 * 8);
		h *= m;
	};

	h ^= h >> r;
	h *= m;
	h ^= h >> r;

	return h;
}


