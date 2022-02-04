#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include <new>
#include "allocator_base.h"

namespace MyAllocator{
	namespace ArenaAllocatorImpl{

		struct MinimalSTDAllocator{
			static void *xallocate(std::size_t const size) noexcept{
				return ::operator new(size, std::nothrow);
			}

			static void xdeallocate(void *p) noexcept{
				return ::operator delete(p);
			}
		};

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

				std::byte *result = buffer.data() + pos;

				pos += size;

				return result;
			}

			constexpr
			static void xdeallocate(void *) noexcept{
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
			Buffer		buffer;

			std::size_t	pos	= 0;
		};

		struct RawBuffer{
			RawBuffer(std::byte *data, std::size_t size) :
						data_(data),
						size_(size){}

			std::byte *data(){
				return data_;
			}

			auto size() const{
				return size_;
			}

		private:
			std::byte 	*data_;
			std::size_t	size_;
		};

		template<std::size_t Size>
		struct StaticBuffer{
			std::byte *data(){
				return data_;
			}

			constexpr static auto size(){
				return Size;
			}

		private:
			std::byte 	data_[Size];
		};

		template<class Allocator = MinimalSTDAllocator>
		struct DynamicBuffer{
			template<class ...Args>
			DynamicBuffer(std::size_t size, Args &&...args) :
						allocator_( std::forward<Args>(args)...		),
						data_(allocate<std::byte>(allocator_, size)	),
						size_(size					){

				if (data_ == nullptr)
					throw std::bad_alloc{};
			}

			~DynamicBuffer(){
				deallocate(allocator_, data_);
			}

			std::byte *data(){
				return data_;
			}

			auto size() const{
				return size_;
			}

		private:
			Allocator	allocator_;
			std::byte 	*data_;
			std::size_t	size_;
		};

	} // ArenaAllocatorImpl

	using ArenaAllocatorRaw		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::RawBuffer>;

	using ArenaAllocator		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::DynamicBuffer<> >;

	template<std::size_t Size>
	using ArenaAllocatorStatic	= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::StaticBuffer<Size> >;

} // namespace MyAllocator

#endif

