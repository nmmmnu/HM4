#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include "baseallocator.h"
#include "allocatedbuffer.h"

namespace MyAllocator{
	namespace ArenaAllocatorImpl{

		using PtrType = std::byte;

		template<class Buffer>
		struct ArenaAllocatorBase{
			constexpr static const char *getName(){
				return "ArenaAllocator";
			}

			template<typename... Args>
			ArenaAllocatorBase(Args&&... args) : buffer(std::forward<Args>(args)...){}

			constexpr static std::size_t DEFAULT_ALIGN = sizeof(void *);

			template<std::size_t Align = DEFAULT_ALIGN>
			void *xallocate(std::size_t const size) noexcept{
				pos = align_(pos, Align);

				if (pos + size > buffer.size())
					return nullptr;

				PtrType *result = buffer.data() + pos;

				pos += size;

				return result;
			}

			constexpr
			static void xdeallocate(void *) noexcept{
			}

			bool owns(const void *p) const{
				return
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
			constexpr static std::size_t align_(std::size_t n, std::size_t align = DEFAULT_ALIGN) {
				return (n + align - 1) & ~(align - 1);
			}

		private:
			Buffer		buffer;

			std::size_t	pos	= 0;
		};



		using LinkedBuffer		= MyBuffer::LinkedBuffer<PtrType>;

		template<std::size_t Size>
		using  StaticBuffer		= MyBuffer::StaticBuffer<PtrType, Size>;

		template<class Allocator = std::nullptr_t>
		using AllocatedByteBuffer	= MyBuffer::AllocatedBuffer<PtrType, Allocator>;
	} // ArenaAllocatorImpl

	using ArenaAllocatorRaw		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::LinkedBuffer>;

	using ArenaAllocator		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::AllocatedByteBuffer<> >;

	template<std::size_t Size>
	using ArenaAllocatorStatic	= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::StaticBuffer<Size> >;

} // namespace MyAllocator

#endif

