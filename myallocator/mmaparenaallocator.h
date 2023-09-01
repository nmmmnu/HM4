#ifndef MMAP_ARENA_ALLOCATOR_H_
#define MMAP_ARENA_ALLOCATOR_H_

#include "arenaallocator.h"
#include "mmapallocator.h"

namespace MyAllocator{

	using MMapArenaAllocatorInplace	= ArenaAllocatorT	<MMapAllocator>;
	using MMapArenaAllocator	= ArenaAllocatorLinked	<MMapAllocator>;

} // namespace MyAllocator

#endif

