#ifndef MY_CPP_ALLOCATOR
#define MY_CPP_ALLOCATOR

#include  <cstddef>

namespace MyAllocator{

	struct STDAllocator{
		constexpr static const char *getName(){
			return "STDAllocator";
		}

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

		constexpr static std::size_t getFreeMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		constexpr static std::size_t getUsedMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		template<typename T>
		static auto wrapInSmartPtr(T *p) noexcept{
			return std::unique_ptr<T>{ p };
		}
	};

} // namespace MyAllocator

#endif

