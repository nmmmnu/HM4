#ifndef MY_NULL_ALLOCATOR
#define MY_NULL_ALLOCATOR

#include <cstddef>
#include <limits>

namespace MyAllocator{

	struct NULLAllocator{
		constexpr
		static void *allocate(std::size_t) noexcept{
			return nullptr;
		}

		constexpr
		static void deallocate(void *) noexcept{
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

		constexpr static bool reset() noexcept{
			return true;
		}

		constexpr static std::size_t getFreeMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		constexpr static std::size_t getUsedMemory() noexcept{
			return std::numeric_limits<std::size_t>::max();
		}

		template<typename T>
		static auto wrapInSmartPtr(T *p) noexcept{
			auto deleter = [](){
			};

			return std::unique_ptr<T, decltype(deleter)>{
				p,
				deleter
			};
		}
	};

} // namespace MyAllocator

#endif

