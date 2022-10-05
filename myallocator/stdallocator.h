#ifndef MY_CPP_ALLOCATOR
#define MY_CPP_ALLOCATOR

#include  <cstddef>
#include "baseallocator.h"

namespace MyAllocator{

	struct STDAllocator{
		constexpr static const char *getName(){
			return "STDAllocator";
		}

		static void *xallocate(std::size_t const size) noexcept{
			return ::operator new(size, std::nothrow);
		}

		static void xdeallocate(void *p) noexcept{
			return ::operator delete(p);
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
	};

	constexpr STDAllocator var_STDAllocator;



	template<typename T>
	inline auto wrapInSmartPtr(STDAllocator &, T *p) noexcept{
		return std::unique_ptr<T>{ p };
	}

} // namespace MyAllocator

#endif

