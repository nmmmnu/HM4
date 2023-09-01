#include "mmapallocator.h"

#include "logger.h"

#include <sys/mman.h>
#include <cassert>
#include <cstdio>

namespace MyAllocator{
	namespace{
		// duplicate, but with template looks very bad.

		void *mmapNormal_(std::size_t const size) noexcept{
			auto const options = MAP_PRIVATE | MAP_ANONYMOUS;

			void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, options, -1, 0);

			if (p == MAP_FAILED)
				return nullptr;
			else
				return p;
		}


		#ifdef USE_HUGETLB

		void *mmapHugeTLB(std::size_t const size) noexcept{
			constexpr std::string_view mask = "MMapAllocator allocating {} bytes with {} mmap.";

			auto const options = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;

			void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, options, -1, 0);

			if (p == MAP_FAILED){
				logger<Logger::WARNING>() << "MMapAllocator allocating HUGETLB fail, going back to conventional memory.";

				logger_fmt<Logger::NOTICE>(mask, size, "conventional");

				return mmapNormal_(size);
			}else{
				logger_fmt<Logger::NOTICE>(mask, size, "HugeTLB");

				return p;
			}
		}

		#endif
	}

	void *MMapAllocator::xallocate(std::size_t const size) noexcept{
		assert(size == size_);

		#ifdef USE_HUGETLB
			return mmapHugeTLB(size_);
		#else
			return mmapNormal_(size_);
		#endif
	}

	void MMapAllocator::xdeallocate(void *p) noexcept{
		munmap(p, size_);
	}


} // namespace MyAllocator::hugetlb_impl_


