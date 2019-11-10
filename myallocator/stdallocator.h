#ifndef MY_CPP_ALLOCATOR
#define MY_CPP_ALLOCATOR

#include  <cstddef>

namespace MyAllocator{

	struct STDAllocator{
		static void *allocate(std::size_t const size) noexcept{
			return ::operator new(size, std::nothrow);
		}

		static void deallocate(void *p) noexcept{
			return ::operator delete(p);
		}

		constexpr static bool need_deallocate() noexcept{
			return true;
		}

		constexpr static bool reset() noexcept{
			return false;
		}
	};

} // namespace MyAllocator

#endif

