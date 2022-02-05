#ifndef MY_STACK_ALLOCATOR
#define MY_STACK_ALLOCATOR

#include <cstdlib>
#include <limits>
#include "baseallocator.h"

namespace MyAllocator{

	struct MallocAllocator{
		constexpr static const char *getName(){
			return "StackAllocator";
		}

		static void *xallocate(std::size_t const size) noexcept{
			return alloca(size);
		}

		constexpr static void xdeallocate(void *) noexcept{
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return false;
		}

		constexpr static bool reset() noexcept{
			return true;
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

