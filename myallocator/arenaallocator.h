#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include <memory>

namespace MyAllocator{

	struct ArenaAllocatorRaw{
		constexpr static const char *getName(){
			return "ArenaAllocator";
		}

		ArenaAllocatorRaw(std::byte *data, std::size_t size) :
						data(data),
						size(size){}

		constexpr static std::size_t DEFAULT_ALIGN = sizeof(void *);

		template<std::size_t Align = DEFAULT_ALIGN>
		void *allocate(std::size_t const size) noexcept{
			pos = align_(pos, Align);

			if (pos + size > this->size)
				return nullptr;

			std::byte *result = data + pos;

			pos += size;

			return result;
		}

		constexpr
		static void deallocate(void *) noexcept{
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

		bool reset() noexcept{
			pos = 0;
			return true;
		}

		std::size_t getFreeMemory() const noexcept{
			return size - pos;
		}

		std::size_t getUsedMemory() const noexcept{
			return pos;
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

	private:
		// http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator
		constexpr static std::size_t align_(std::size_t n, std::size_t align = DEFAULT_ALIGN) {
			return (n + align - 1) & ~(align - 1);
		}

	private:
		std::byte	*data;
		std::size_t	size;
		std::size_t	pos	= 0;
	};

	namespace ArenaAllocatorImpl{
		struct DynamicBuffer{
			DynamicBuffer(std::size_t size) : buffer{ std::make_unique<std::byte[]>(size) }{}

			std::unique_ptr<std::byte[]> buffer;
		};

		template<std::size_t Size>
		struct StaticBuffer{
			std::byte buffer[Size];
		};
	} // ArenaAllocatorImpl

	struct ArenaAllocator : private ArenaAllocatorImpl::DynamicBuffer, public ArenaAllocatorRaw{
		ArenaAllocator(std::size_t size) :
					DynamicBuffer(size),
					ArenaAllocatorRaw(buffer.get(), size){}
	};

	template<std::size_t Size>
	struct ArenaAllocatorStatic : private ArenaAllocatorImpl::StaticBuffer<Size>, public ArenaAllocatorRaw{
		using ArenaAllocatorImpl::StaticBuffer<Size>::buffer;

		ArenaAllocatorStatic() :
					ArenaAllocatorRaw(buffer, Size){}
	};

} // namespace MyAllocator

#endif

