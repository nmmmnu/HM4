#ifndef MY_SIMULATED_ARENA_ALLOCATOR
#define MY_SIMULATED_ARENA_ALLOCATOR

#include <cstdlib>
#include <limits>
#include "baseallocator.h"

#include <malloc.h>

namespace MyAllocator{

	struct SimulatedArenaAllocator{
		SimulatedArenaAllocator(size_t arena) : arena(arena){}

		constexpr static const char *getName(){
			return "SimulatedArenaAllocator";
		}

		void *xallocate(std::size_t const size) noexcept{
			if (allocated + size + sizeof(void *) > arena)
				return nullptr;

			auto p = malloc(size);

			allocated += malloc_usable_size(p);

			return p;
		}

		void xdeallocate(void *p) noexcept{
			allocated -= malloc_usable_size(p);

			return free(p);
		}

		constexpr static bool need_deallocate() noexcept{
			return true;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return true;
		}

		constexpr static bool reset() noexcept{
			return false;
		}

		std::size_t getFreeMemory() const noexcept{
			return arena - allocated;
		}

		std::size_t getUsedMemory() const noexcept{
			return allocated;
		}

	private:
		size_t arena;
		size_t allocated = 0;
	};

} // namespace MyAllocator

#endif

