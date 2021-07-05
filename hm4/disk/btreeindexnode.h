#ifndef BTREE_INDEX_NODE_H
#define BTREE_INDEX_NODE_H

#include "bfslookup.h"

#include <cstdint>
#include <limits>

namespace hm4{
namespace disk{
namespace btree{



using level_type	= uint8_t;



constexpr level_type	NODE_LEVELS	= 7;

constexpr auto LL = BFSLookupFactory<level_type, btree::NODE_LEVELS>::build();

constexpr level_type	VALUES		= LL.size();



struct NodeData{
	uint64_t	dataid;			// 8
	uint16_t	keysize;		// 2

public:
	constexpr static size_t ALIGN = sizeof(uint64_t);

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

