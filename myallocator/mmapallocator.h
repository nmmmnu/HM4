#ifndef MMAP_ALLOCATOR_H_
#define MMAP_ALLOCATOR_H_

#include "baseallocator.h"

namespace MyAllocator{

	struct MMapAllocator{
		MMapAllocator(size_t size) : size_(size){}

		constexpr static const char *getName(){
			return "HugeTLBAllocator";
		}

		constexpr std::size_t size() const noexcept{
			return size_;
		}

		void *xallocate(std::size_t const size) noexcept;

		void xdeallocate(void *p) noexcept;

		constexpr static bool owns(const void *) noexcept{
			return false;
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

	private:
		std::size_t size_;
	};

} // namespace MyAllocator

#endif

