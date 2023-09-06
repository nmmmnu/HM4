#ifndef MMAP_ALLOCATOR_H_
#define MMAP_ALLOCATOR_H_

#include "baseallocator.h"

#include <cassert>

namespace MyAllocator{

	namespace mmapallocator_impl_{
		void *createNormal(std::size_t size) noexcept;
		#ifdef USE_HUGETLB
		void *createHugeTLB(std::size_t size) noexcept;
		#endif
		void destroy(void *p, std::size_t size_) noexcept;
	};

	struct MMapAllocator{
		constexpr static const char *getName(){
			return "MMapAllocator";
		}

		void *xallocate(std::size_t const size) noexcept{
			assert(size_ == 0);

			void *p = (
				#ifdef USE_HUGETLB
					mmapallocator_impl_::createHugeTLB(size)
				#else
					mmapallocator_impl_::createNormal(size)
				#endif
			);

			size_ = p ? size : 0;

			return p;
		}

		void xdeallocate(void *p) noexcept{
			assert(size_);

			mmapallocator_impl_::destroy(p, size_);

			size_ = 0;
		}

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
		std::size_t size_ = 0;
	};

} // namespace MyAllocator

#endif

