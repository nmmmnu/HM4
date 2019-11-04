#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include <cstddef>
#include <memory>

namespace MyAllocator{

	template<std::size_t SIZE>
	struct ArenaAllocator{
		void reset() noexcept{
			pos = 0;
		}

		template<class T>
		bool free(size_t const size = sizeof(T) ) noexcept{
			return pos + size < SIZE;
		}

		template<class T>
		T *allocate(size_t const size = sizeof(T) ) noexcept{
			if (pos + size > SIZE)
				return nullptr;

			std::byte *result = data + pos;

			pos += size;

			return reinterpret_cast<T *>( result );
		}

		constexpr
		static void deallocate(void *) noexcept{
		}

		template<class T>
		static auto getUP(T *p) noexcept{
			return p;
		}

		constexpr
		static auto getDeallocate() noexcept{
			return [](void *){};
		}

	private:
		template<class T>
		[[maybe_unused]]
		static auto getUP__(T *p){
			auto x = getDeallocate();
			return std::unique_ptr<T, decltype(x)>{ p, x };
		}

	private:
		std::size_t	pos = 0;
		std::byte	data[SIZE];
	};

} // namespace MyAllocator

#endif

