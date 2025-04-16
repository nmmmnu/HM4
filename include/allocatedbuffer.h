#ifndef MY_ALLOCATED_BUFFER_
#define MY_ALLOCATED_BUFFER_

#include "baseallocator.h"

namespace MyBuffer{

	template<class Allocator = std::nullptr_t>
	struct AllocatedBufferOwnedResource{
		using value_type = char;
		using size_type  = std::size_t;

		template<class ...Args>
		AllocatedBufferOwnedResource(size_type size, Args &&...args) :
					allocator_	(std::forward<Args>(args)...	),
					size_		(size				){}

		AllocatedBufferOwnedResource(size_type size, std::nullptr_t allocator) :
					allocator_	(allocator		),
					size_		(size			){}

		AllocatedBufferOwnedResource(AllocatedBufferOwnedResource &other) :
					size_		(other.size_		),
					data_		(other.data_		){

			other.size_ = 0;
			other.data_ = nullptr;
		}

		~AllocatedBufferOwnedResource(){
			MyAllocator::deallocate(allocator, data_);
		}

		operator bool() const noexcept{
			return data_;
		}

		value_type *data() noexcept{
			return data_.get();
		}

		const value_type *data() const noexcept{
			return data_.get();
		}

		size_type size() const noexcept{
			return size_;
		}

	private:
		Allocator	allocator_;
		size_type	size_;
		value_type	data_	= MyAllocator::allocate<char>(allocator, size_);
	};



	template<typename T, class Allocator = std::nullptr_t>
	struct AllocatedBufferLinkedResource{
		using value_type = char;
		using size_type  = std::size_t;

		AllocatedBufferLinkedResource(size_type size, Allocator &allocator) :
					allocator_	(& allocator	),
					size_		(size		){}

		AllocatedBufferLinkedResource(size_type size, std::nullptr_t) :
					allocator_	(nullptr	),
					size_		(size		){}

		AllocatedBufferLinkedResource(AllocatedBufferLinkedResource &other) :
					allocator_	(other.allocator_	),
					size_		(other.size_		),
					data_		(other.data_		){

			other.size_ = 0;
			other.data_ = nullptr;
		}

		~AllocatedBufferLinkedResource(){
			MyAllocator::deallocate(allocator, data_);
		}

		operator bool() const noexcept{
			return data_;
		}

		value_type *data() noexcept{
			return data_.get();
		}

		const value_type *data() const noexcept{
			return data_.get();
		}

		size_type size() const noexcept{
			return size_;
		}

	private:
		Allocator	*allocator_;
		size_type	size_;
		char		data_	= MyAllocator::allocate<char>(allocator, size_);
	};

} // namespace MyBuffer

#endif

