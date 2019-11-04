#ifndef MY_MALLOC_ALLOCATOR
#define MY_MALLOC_ALLOCATOR

#include <cstdlib>
#include <memory>

namespace MyAllocator{

	struct MallocAllocator{
		template<class T>
		static bool free(size_t = sizeof(T) ) noexcept{
			return true;
		}

		template<class T>
		static T *allocate(size_t size = sizeof(T) ) noexcept{
			return reinterpret_cast<T *>( malloc(size) );
		}

		static void deallocate(void *p) noexcept{
			return ::free(p);
		}

		constexpr
		static auto getDeallocate() noexcept{
			return [](void *p){
				deallocate(p);
			};
		}

		template<class T>
		static auto getUP(T *p){
			auto x = getDeallocate();

			return std::unique_ptr<T, decltype(x)>{ p, x };
		}
	};

} // namespace MyAllocator

#endif

