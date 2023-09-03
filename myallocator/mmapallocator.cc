#include "mmapallocator.h"

#include "logger.h"

#include <sys/mman.h>

namespace MyAllocator{
	namespace mmapallocator_impl_{
		// duplicate, but with template looks very bad.

		void *createNormal(std::size_t const size) noexcept{
			auto const options = MAP_PRIVATE | MAP_ANONYMOUS;

			void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, options, -1, 0);

			if (p == MAP_FAILED)
				return nullptr;
			else
				return p;
		}


		#ifdef USE_HUGETLB

		void *createHugeTLB(std::size_t const size) noexcept{
			constexpr std::string_view mask = "MMapAllocator allocating {} bytes with {} mmap.";

			auto const options = MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB;

			void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, options, -1, 0);

			if (p != MAP_FAILED){
				logger_fmt<Logger::NOTICE>(mask, size, "HugeTLB");

				return p;
			}

			logger<Logger::WARNING>() << "MMapAllocator allocating with HugeTLB mmap fail, going back to conventional memory.";

			logger_fmt<Logger::NOTICE>(mask, size, "conventional");

			return createNormal(size);
		}

		#endif

		void destroy(void *p, std::size_t size) noexcept{
			constexpr std::string_view mask = "MMapAllocator deallocating {} bytes with mmap.";

			logger_fmt<Logger::NOTICE>(mask, size);

			munmap(p, size);
		}

	} // namespace mmapallocator_impl_
} // namespace MyAllocator


