#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include  <cstddef>
#include  <memory>

namespace MyAllocator{

	struct ArenaAllocator{
		ArenaAllocator(std::size_t const maxsize) :
						maxsize(maxsize),
						data( allocate__(maxsize) ){}

		void reset() noexcept{
			pos = 0;
		}

		bool free(std::size_t const size) const noexcept{
			return pos + size < maxsize;
		}



		template<std::size_t Align = sizeof(void *)>
		void *allocate(std::size_t const size) noexcept{
			pos = align(pos, Align);

			if (pos + size > maxsize)
				return nullptr;

			std::byte *result = data.get() + pos;

			pos += size;

			return result;
		}

		constexpr
		static void deallocate(void *) noexcept{
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

	private:
		// http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator
		constexpr static std::size_t align(std::size_t n, std::size_t align = sizeof(void *)) {
			return (n + align - 1) & ~(align - 1);
		}

		static std::byte *allocate__(std::size_t const maxsize){
			return static_cast<std::byte *>(
				::operator new(maxsize)
			);
		}

	private:
		std::size_t			pos = 0;
		std::size_t			maxsize;
		std::unique_ptr<std::byte>	data;
	};

} // namespace MyAllocator

#endif
