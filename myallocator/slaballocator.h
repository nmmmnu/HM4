#ifndef SLAB_ALLOCATOR
#define SLAB_ALLOCATOR

#include "baseallocator.h"
#include "mybufferview.h"

namespace MyAllocator{

	template<size_t BlockSize>
	struct SlabAllocator{
		static_assert(BlockSize > sizeof(void *), "No room for the free list");

		constexpr static const char *getName(){
			return "SlabAllocator";
		}

		constexpr static bool need_deallocate() noexcept{
			return false;
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

		size_t getFreeBlocks() const noexcept{
			size_t n = 0;

			for(const void *block = freeList_; block; block = *reinterpret_cast<const void * const *>(block))
				++n;

			return n;
		}

		size_t getUsedBlocks() const noexcept{
			return getNumBlocks() - getFreeBlocks();
		}

		size_t getFreeMemory() const noexcept{
			return getFreeBlocks() * BlockSize;
		}

		size_t getUsedMemory() const noexcept{
			return getUsedBlocks() * BlockSize;
		}

		constexpr auto getNumBlocks() const noexcept{
			return buffer_.size() / BlockSize;
		}

	private:
		void createFreeList_(){
			for (size_t i = 0; i < getNumBlocks(); ++i){
				void *block = buffer_.data() + i * BlockSize;
				*reinterpret_cast<void **>(block) = freeList_;
				freeList_ = block;
			}
		}

	private:
		MyBuffer::ByteBufferView	buffer_;
		void				*freeList_	= nullptr;
	};

} // namespace MyAllocator

#endif
