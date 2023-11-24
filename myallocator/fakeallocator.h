#ifndef MY_FAKE_ALLOCATOR_
#define MY_FAKE_ALLOCATOR_

#include <limits>

namespace MyAllocator{

	struct FakeAllocator{
		constexpr FakeAllocator(void *mem) : mem(mem){}

		constexpr static const char *getName(){
			return "FakeAllocator";
		}

		constexpr void *xallocate(std::size_t) noexcept{
			return mem;
		}

		constexpr static void xdeallocate(void *p) noexcept{
		}

		constexpr static bool owns(const void *) noexcept{
			return false;
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

	private:
		void *mem;
	};

} // namespace MyAllocator

#endif

