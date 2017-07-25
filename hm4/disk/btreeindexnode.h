#ifndef BTREE_INDEX_NODE_H
#define BTREE_INDEX_NODE_H

#include "levelorderlookup.h"

#include <cstdint>
#include <limits>

namespace hm4{
namespace disk{
namespace btree{


using level_type	= uint16_t;


constexpr level_type	NODE_LEVELS	= 7;
constexpr level_type	BRANCHES	= 1 << NODE_LEVELS;
constexpr level_type	VALUES		= BRANCHES - 1;


constexpr LevelOrderLookup<NODE_LEVELS>	LL;


struct NodeData{
	uint64_t	dataid;			// 8
	uint16_t	keysize;		// 2
} __attribute__((__packed__));


struct Node{
	uint64_t	values[VALUES];		// 8 * VALUES

public:
	constexpr static uint64_t NIL		= std::numeric_limits<uint64_t>::max();
} __attribute__((__packed__));


} // namespace btree
} // namespace disk
} // namespace


#endif

