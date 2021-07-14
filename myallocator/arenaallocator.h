#ifndef MY_ARENA_ALLOCATOR
#define MY_ARENA_ALLOCATOR

#include <memory>

namespace MyAllocator{
	namespace ArenaAllocatorImpl{

		template<class Buffer>
		struct ArenaAllocatorBase{
			constexpr static const char *getName(){
				return "ArenaAllocator";
			}

			template<typename... Args>
			ArenaAllocatorBase(Args&&... args) : buffer(std::forward<Args>(args)...){}

			constexpr static std::size_t DEFAULT_ALIGN = sizeof(void *);

			template<std::size_t Align = DEFAULT_ALIGN>
			void *allocate(std::size_t const size) noexcept{
				pos = align_(pos, Align);

				if (pos + size > buffer.size())
					return nullptr;

				std::byte *result = buffer.data() + pos;

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

		struct DynamicBuffer{
			DynamicBuffer(std::size_t size) :
						size_(size){}

			std::byte *data(){
				return data_.get();
			}

			auto size() const{
				return size_;
			}

		private:
			std::size_t			size_;
			std::unique_ptr<std::byte[]>	data_ = std::make_unique<std::byte[]>(size_);
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

	} // ArenaAllocatorImpl


	using ArenaAllocatorRaw		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::RawBuffer>;

	using ArenaAllocator		= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::DynamicBuffer>;

	template<std::size_t Size>
	using ArenaAllocatorStatic	= ArenaAllocatorImpl::ArenaAllocatorBase<ArenaAllocatorImpl::StaticBuffer<Size> >;

} // namespace MyAllocator

#endif

