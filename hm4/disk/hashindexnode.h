#ifndef HASH_INDEX_NODE_H_
#define HASH_INDEX_NODE_H_

#include <cstdint>

#include "murmur_hash_64a.h"

namespace hm4::disk::hash{

	constexpr static uint8_t HASHTABLE_OPEN_ADDRESSES	= 16;

	struct Node{
		uint64_t hash;
		uint64_t pos;
	}
	__attribute__((__packed__));

} // namespace hm4::disk::hash

#endif

