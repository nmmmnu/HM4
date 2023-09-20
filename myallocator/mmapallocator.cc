#include "mmapallocator.h"

#include "logger.h"

#include <sys/mman.h>

namespace MyAllocator{
	namespace mmapallocator_impl_{
		// duplicate, but with template looks very bad.

		constexpr std::string_view maskAllocate   = "MMapAllocator allocating {} bytes with {} mmap.";
		constexpr std::string_view maskDeallocate = "MMapAllocator deallocating {} bytes with mmap.";

		inline void *mmap_(std::size_t const size, int options = 0){
			return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | options, -1, 0);
		}

		void *createNormal(std::size_t const size) noexcept{
			if (void *p = mmap_(size); p != MAP_FAILED){
				logger_fmt<Logger::NOTICE>(maskAllocate, size, "conventional");
				return p;
			}

			return nullptr;
		}


		#ifdef USE_HUGETLB

		void *createHugeTLB(std::size_t const size) noexcept{
			if (void *p = mmap_(size, MAP_HUGETLB); p != MAP_FAILED){
				logger_fmt<Logger::NOTICE>(maskAllocate, size, "HugeTLB");
				return p;
			}

			logger<Logger::WARNING>() << "MMapAllocator allocating with HugeTLB mmap fail, going back to conventional memory.";
			return createNormal(size);
		}

		#endif

		void destroy(void *p, std::size_t size) noexcept{
			logger_fmt<Logger::NOTICE>(maskDeallocate, size);

			munmap(p, size);
		}

	} // namespace mmapallocator_impl_
} // namespace MyAllocator


