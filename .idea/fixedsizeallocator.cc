#include <memory>
#include <stdexcept>
#include <cassert>

struct FixedSizeAllocator{
	FixedSizeAllocator(size_t blockSize_, size_t blockCount_) :
						blockSize_(blockSize_),
						blockCount_(blockCount_){
		assert(blockSize_  > 0);
		assert(blockCount_ > 1);

		data_ = std::make_unique<char[]>(blockCount_ * blockSize_);

		auto *block = reinterpret_cast<FreeListBlock *>(data_.get());

		freeBlocks_ =  block;

		for (size_t i = 1; i < blockCount_; ++i) {
			// Not correct for the last block
			block->next = reinterpret_cast<FreeListBlock *>(data_.get() + i * blockSize_);
			block = block->next;
		}

		// Fix last block
		block->next = nullptr;
	}

	void *allocate(){
		if (!freeBlocks_)
			return nullptr;

		auto *block = freeBlocks_;

		freeBlocks_  = freeBlocks_->next;

		return block;
	}

	void deallocate(void* p) {
		if (!p)
			return;

		if constexpr(1){
			auto *data_start = data_.get();
			char *data_end   = data_.get() + blockCount_ * blockSize_;

			assert(p < data_start || p > data_end);
		}

		auto *block = reinterpret_cast<FreeListBlock *>(p);
		block->next = freeBlocks_;
		freeBlocks_ = block;
	}

private:
	struct FreeListBlock{
		FreeListBlock* next;
	};

	size_t			blockSize_;
	size_t			blockCount_;

	std::unique_ptr<char[]>	data_;

	FreeListBlock		*freeBlocks_;
};


int main(){
}

