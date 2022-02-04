#ifndef MY_MALLOC_ALLOCATOR
#define MY_MALLOC_ALLOCATOR

#include <cstdlib>
#include <limits>
#include "allocator_base.h"

namespace MyAllocator{

	struct MallocAllocator{
		constexpr static const char *getName(){
			return "MallocAllocator";
		}

		static void *xallocate(std::size_t const size) noexcept{
			return malloc(size);
		}

		static void xdeallocate(void *p) noexcept{
			return free(p);
		}

		constexpr static bool need_deallocate() noexcept{
			return true;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return false;
		}

		constexpr static bool reset() noexcept{
			return false;
		}

		constexpr static std::size_t getFreeMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		constexpr static std::size_t getUsedMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}
	};

} // namespace MyAllocator

#endif

