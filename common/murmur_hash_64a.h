#ifndef MURMUR_HASH_64A_H_
#define MURMUR_HASH_64A_H_

#include <cstdint>
#include <string_view>

uint64_t murmur_hash64a(const void *key, size_t size, uint64_t seed = 0);

inline uint64_t murmur_hash64a(std::string_view s, uint64_t seed = 0){
	return murmur_hash64a(s.data(), s.size(), seed);
}

#endif

