#ifndef SLAB_ALLOCATOR
#define SLAB_ALLOCATOR

#include "baseallocator.h"
#include "mybufferview.h"

namespace MyAllocator{

	template<size_t BlockSize = 2048>
	struct SlabAllocator{
		static_assert(BlockSize > sizeof(void *), "No room for the free list");

		constexpr static const char *getName(){
			return "SlabAllocator";
		}

		constexpr static bool need_deallocate() noexcept{
			return true;
		}

		constexpr static bool knownMemoryUsage() noexcept{
			return true;
		}

		template<typename... Args>
		SlabAllocator(Args&&... args) : buffer_(std::forward<Args>(args)...){
			createFreeList_();
		}

		bool reset(){
			freeList_ = nullptr;
			createFreeList_();
			return true;
		}

		void *xallocate(size_t size){
			if (size > BlockSize)
				return nullptr;

			return allocate();
		}

		void *allocate(){
			if (!freeList_)
				return nullptr;

			void *block = freeList_;

			freeList_ = *reinterpret_cast<void **>(freeList_);

			return block;
		}

		void xdeallocate(void* p){
			*reinterpret_cast<void **>(p) = freeList_;
			freeList_ = p;
		}

		size_t getFreeMemory() const noexcept{
			size_t n = 0;

			for(const void *block = freeList_; block; block = *reinterpret_cast<const void * const *>(block))
				++n;

			return n * BlockSize;
		}

		size_t getUsedMemory() const noexcept{
			return numBlocks_() * BlockSize - getFreeMemory();
		}

	private:
		void createFreeList_(){
			for (size_t i = 0; i < numBlocks_(); ++i){
				void *block = buffer_.data() + i * BlockSize;
				*reinterpret_cast<void **>(block) = freeList_;
				freeList_ = block;
			}
		}

		constexpr auto numBlocks_() const{
			return buffer_.size() / BlockSize;
		}

	private:
		MyBuffer::ByteBufferView	buffer_;
		void				*freeList_	= nullptr;
	};

} // namespace MyAllocator

#endif
