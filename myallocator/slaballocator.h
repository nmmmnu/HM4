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
		SlabAllocator(Args &&...args) : buffer_(std::forward<Args>(args)...){
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

			return xallocate();
		}

		void *xallocate(){
			if (!freeList_)
				return nullptr;

			void *block = freeList_;

			freeList_ = *reinterpret_cast<void **>(freeList_);

			return block;
		}

		void xdeallocate(void *p){
			if (!owns_(p))
				return;

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

		constexpr auto const &getBuffer_() const{
			return buffer_;
		}

	private:
		bool owns_(const void *p) const{
			const char *pp = static_cast<const char *>(p);

			return pp >= buffer_.data() && pp < buffer_.data() + buffer_.size();
		}

		void createFreeList_(){
			if (0){
				for (size_t i = 0; i < getNumBlocks(); ++i){
					void *block = buffer_.data() + i * BlockSize;
					*reinterpret_cast<void **>(block) = freeList_;
					freeList_ = block;
				}
			}else{
				for (size_t i = getNumBlocks(); i --> 0; ){
					void *block = buffer_.data() + i * BlockSize;
					*reinterpret_cast<void **>(block) = freeList_;
					freeList_ = block;
				}
			}
    		}

	private:
		MyBuffer::ByteBufferView	buffer_;
		void				*freeList_	= nullptr;
	};



	template<size_t BlockSize>
	void *allocate(SlabAllocator<BlockSize> &allocator){
		return allocator.xallocate();
	}

	template<typename T, size_t BlockSize>
	T *allocate(SlabAllocator<BlockSize> &allocator){
		return static_cast<T *>(
			allocate(allocator)
		);
	}

	template<size_t BlockSize>
	void *allocate(SlabAllocator<BlockSize> *allocator){
		return allocate(*allocator);
	}

	template<typename T, size_t BlockSize>
	T *allocate(SlabAllocator<BlockSize> *allocator){
		return allocate<T>(*allocator);
	}

} // namespace MyAllocator

#endif
