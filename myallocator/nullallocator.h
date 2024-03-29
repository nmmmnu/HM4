#ifndef MY_NULL_ALLOCATOR
#define MY_NULL_ALLOCATOR

namespace MyAllocator{

	struct NULLAllocator{
		constexpr static const char *getName(){
			return "NULLAllocator";
		}

		constexpr
		static void *xallocate(std::size_t) noexcept{
			return nullptr;
		}

		constexpr
		static void xdeallocate(void *) noexcept{
		}

		constexpr static bool owns(const void *) noexcept{
			return false;
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return true;
		}

		constexpr static bool reset() noexcept{
			return true;
		}

		constexpr static std::size_t getFreeMemory() noexcept{
			return 0;
		}

		constexpr static std::size_t getUsedMemory() noexcept{
			return 0;
		}
	};

	constexpr NULLAllocator var_NULLAllocator;

} // namespace MyAllocator

#endif

