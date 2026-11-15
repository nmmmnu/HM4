#ifndef SLAB_ALLOCATOR_H_
#define SLAB_ALLOCATOR_H_

#include "baseallocator.h"
#include "mybufferview.h"

#include <type_traits>

#include <cassert>

namespace MyAllocator{

	namespace slab_allocator_impl_{

		struct LinearBlockTracker{
			constexpr static void inc  () noexcept{}
			constexpr static void dec  () noexcept{}
			constexpr static void reset() noexcept{}

			static size_t getFreeBlocks(void *freeList, size_t) noexcept{
				size_t n = 0;

				for(const void *block = freeList; block; block = *reinterpret_cast<const void * const *>(block))
					++n;

				return n;
			}

			static bool getFreeBlocks(size_t needed, void *freeList, size_t) noexcept{
				if (!needed)
					return true;

				for(const void *block = freeList; block; block = *reinterpret_cast<const void * const *>(block))
					if (--needed == 0)
						return true;

				return false;
			}

			static size_t getUsedBlocks(void *freeList, size_t numBlocks) noexcept{
				return numBlocks - getFreeBlocks(freeList, numBlocks);
			}
		};

		struct FastBlockTracker{
			constexpr void inc() noexcept{
				++usedBlocks;
			}

			constexpr void dec() noexcept{
				--usedBlocks;
			}

			constexpr void reset() noexcept{
				usedBlocks = 0;
			}

			constexpr size_t getFreeBlocks(void *, size_t numBlocks) const noexcept{
				return numBlocks - usedBlocks;
			}

			constexpr bool getFreeBlocks(size_t needed, void *, size_t numBlocks) const noexcept{
				return numBlocks - usedBlocks >= needed;
			}

			constexpr size_t getUsedBlocks(void *, size_t) const noexcept{
				return usedBlocks;
			}

		private:
			size_t usedBlocks = 0;
		};

		template<bool B>
		using BlockTracker = std::conditional_t<B, FastBlockTracker, LinearBlockTracker>;

	} // namespace slab_allocator_impl_



	template<size_t BlockSize, bool Tracker = false>
	class SlabAllocator : private slab_allocator_impl_::BlockTracker<Tracker>{

		using BlockTracker = slab_allocator_impl_::BlockTracker<Tracker>;

	public:
		constexpr static size_t BLOCK_SIZE = BlockSize;

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

			BlockTracker::reset();

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

			BlockTracker::inc();

			return block;
		}

		void xdeallocate(void *p){
			assert(owns_(p) && "Trying to deallocate memory not own by the allocator");

			*reinterpret_cast<void **>(p) = freeList_;
			freeList_ = p;

			BlockTracker::dec();
		}

		size_t getFreeBlocks() const noexcept{
			return BlockTracker::getFreeBlocks(freeList_, getNumBlocks());
		}

		bool getFreeBlocks(size_t needed) const noexcept{
			return BlockTracker::getFreeBlocks(needed, freeList_, getNumBlocks());
		}

		size_t getUsedBlocks() const noexcept{
			return BlockTracker::getUsedBlocks(freeList_, getNumBlocks());
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



	template<size_t BlockSize, bool Tracker = false>
	void *allocate(SlabAllocator<BlockSize, Tracker> &allocator){
		return allocator.xallocate();
	}

	template<typename T, size_t BlockSize, bool Tracker = false>
	T *allocate(SlabAllocator<BlockSize, Tracker> &allocator){
		return static_cast<T *>(
			allocate(allocator)
		);
	}

	template<size_t BlockSize, bool Tracker = false>
	void *allocate(SlabAllocator<BlockSize, Tracker> *allocator){
		return allocate(*allocator);
	}

	template<typename T, size_t BlockSize, bool Tracker = false>
	T *allocate(SlabAllocator<BlockSize, Tracker> *allocator){
		return allocate<T>(*allocator);
	}

} // namespace MyAllocator

#endif
