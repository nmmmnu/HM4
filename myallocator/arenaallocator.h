#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include "baseallocator.h"
#include "mybufferview.h"

#include <cstring>
#include <cstdint>

namespace MyAllocator{

	struct ArenaAllocator{
		constexpr static const char *getName(){
			return "ArenaAllocator";
		}

		template<typename... Args>
		ArenaAllocator(Args&&... args) : buffer(std::forward<Args>(args)...){}

		#if USE_MAP_PAGES
		void mapPages(){
			memset(buffer.data(), 0, buffer.size());
		}
		#else
		constexpr static void mapPages(){
		}
		#endif

		constexpr static std::size_t DEFAULT_ALIGN = sizeof(void *);

		template<std::size_t Align = DEFAULT_ALIGN>
		void *xallocate(std::size_t const size) noexcept{
			pos = align__<Align>(pos);

			if (pos + size > buffer.size())
				return nullptr;

			auto *result = buffer.data() + pos;

			pos += size;

			return result;
		}

		template<std::size_t Align = DEFAULT_ALIGN>
		MyBuffer::ByteBufferView xallocateAll() noexcept{
			pos = align__<Align>(pos);

			size_t const size = buffer.size() - pos;

			auto *result = buffer.data() + pos;

			pos += size;

			return { result, size };
		}

		constexpr
		static void xdeallocate(void *) noexcept{
		}

		bool owns(const void *p) const{
			return
				p &&
				p >= buffer.data() &&
				p <  buffer.data() + buffer.size()
			;
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return true;
		}

		bool reset() noexcept{
			pos = 0;
			return true;
		}

		std::size_t getFreeMemory() const noexcept{
			return buffer.size() - pos;
		}

		std::size_t getUsedMemory() const noexcept{
			return pos;
		}

	private:
		// http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator
	//	constexpr static std::size_t align__(std::size_t n, std::size_t align) noexcept{
	//		return (n + align - 1) & ~(align - 1);
	//	}

		template<std::size_t Align>
		constexpr static std::size_t align__(std::size_t n) noexcept{
			return (n + Align - 1) & ~(Align - 1);
		}

	private:
		MyBuffer::ByteBufferView	buffer;
		std::size_t			pos	= 0;
	};



	constexpr void deallocate(ArenaAllocator &, const void *){
	}



	inline MyBuffer::ByteBufferView allocateAll(ArenaAllocator &allocator){
		return allocator.xallocateAll();
	}

	template<typename T>
	MyBuffer::BufferView<T> allocateAll(ArenaAllocator &allocator){
		return allocateAll(allocator);
	}

	inline auto allocateAll(ArenaAllocator *allocator){
		return allocateAll(*allocator);
	}

	template<typename T>
	auto allocateAll(ArenaAllocator *allocator){
		return allocateAll<T>(*allocator);
	}

} // namespace MyAllocator

#endif

