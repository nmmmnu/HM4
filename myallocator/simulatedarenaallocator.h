#ifndef MY_SIMULATED_ARENA_ALLOCATOR
#define MY_SIMULATED_ARENA_ALLOCATOR

#include <malloc.h>
#include "baseallocator.h"

namespace MyAllocator{

	class SimulatedArenaAllocator{
		constexpr static size_t BUFFER = sizeof(void *) * 4;

	public:
		SimulatedArenaAllocator(size_t arena) : arena(arena){}

		constexpr static const char *getName(){
			return "SimulatedArenaAllocator";
		}

		void *xallocate(std::size_t const size) noexcept{
			if (allocated + size + BUFFER > arena)
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

