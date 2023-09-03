#ifndef MMAP_ALLOCATOR_H_
#define MMAP_ALLOCATOR_H_

#include "baseallocator.h"

#include <algorithm>

namespace MyAllocator{

	namespace mmapallocator_impl_{
		void *createNormal(std::size_t size) noexcept;
		#ifdef USE_HUGETLB
		void *createHugeTLB(std::size_t size) noexcept;
		#endif
		void destroy(void *p, std::size_t size_) noexcept;
	};

	template<size_t Size>
	struct MMapAllocator{
		constexpr static const char *getName(){
			return "MMapAllocator";
		}

		void *xallocate(std::size_t const size) noexcept{
			auto it = std::find_if(std::begin(data_), std::end(data_), [](auto &x){
				return !x.ptr;
			});

			if (it == std::end(data_))
				return nullptr;

			#ifdef USE_HUGETLB
				void *p = mmapallocator_impl_::createHugeTLB(size);
			#else
				void *p = mmapallocator_impl_::createNormal(size);
			#endif

			*it = { p, size };

			return p;
		}

		void xdeallocate(void *p) noexcept{
			auto it = std::find_if(std::begin(data_), std::end(data_), [p](auto &x){
				return x.ptr == p;
			});

			if (it != std::end(data_)){
				mmapallocator_impl_::destroy(p, it->size);
				*it = VS{};
			}
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
		struct VS{
			void	*ptr	= nullptr;
			size_t	size	= 0;
		};

		VS data_[Size];
	};

} // namespace MyAllocator

#endif

