#include "mmapbuffer.h"

#include "logger.h"

#include <sys/mman.h>

#include <cassert>

namespace MyBuffer{

	namespace {
		// duplicate, but with template looks very bad.

		constexpr std::string_view maskAllocate   = "MMapBuffer allocating {} bytes with {} mmap.";
		constexpr std::string_view maskDeallocate = "MMapBuffer deallocating {} bytes with mmap.";

		void *mmap_(std::size_t const size, int options = 0) noexcept{
			return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | options, -1, 0);
		}

		void *createNormal_(std::size_t const size) noexcept{
			if (void *p = mmap_(size); p != MAP_FAILED){
				logger_fmt<Logger::NOTICE>(maskAllocate, size, "conventional");
				return p;
			}

			return nullptr;
		}


		#ifdef USE_HUGETLB

		void *createHugeTLB_(std::size_t const size) noexcept{
			if (void *p = mmap_(size, MAP_HUGETLB); p != MAP_FAILED){
				logger_fmt<Logger::NOTICE>(maskAllocate, size, "HugeTLB");
				return p;
			}

			logger<Logger::WARNING>() << "MMapBuffer allocating with HugeTLB mmap fail, going back to conventional memory.";
			return createNormal_(size);
		}

		#endif
	} // anonymous namespace



	namespace mmapbuffer_impl_{

		void *create(std::size_t size) noexcept{
			assert(size);

			return
				#ifdef USE_HUGETLB
					createHugeTLB_(size)
				#else
					createNormal_(size)
				#endif
			;
		}

		void destroy(void *p, std::size_t size) noexcept{
			if (!p || size == 0)
				return;

			logger_fmt<Logger::NOTICE>(maskDeallocate, size);

			munmap(p, size);
		}



		void adviceNeed(void *p, std::size_t size) noexcept{
			if (!p || size == 0)
				return;

			logger<Logger::NOTICE>() << "MMapBuffer advising MADV_WILLNEED.";
			madvise(p, size, MADV_WILLNEED);
		}

		void adviceFree(void *p, std::size_t size) noexcept{
			if (!p || size == 0)
				return;

			logger<Logger::NOTICE>() << "MMapBuffer advising MADV_DONTNEED.";
			madvise(p, size, MADV_DONTNEED);
		}

	} // namespace mmapallocator_impl_

} // namespace MyAllocator


