#ifndef MY_ALLOCATED_BUFFER_
#define MY_ALLOCATED_BUFFER_

#include "baseallocator.h"

namespace MyBuffer{

	template<class Allocator = std::nullptr_t>
	struct AllocatedMemoryResourceOwned{
		using value_type = void;
		using size_type  = std::size_t;

		template<class ...Args>
		AllocatedMemoryResourceOwned(std::size_t size, Args &&...args) :
					allocator_	(std::forward<Args>(args)...	),
					size_		(size				){}

		AllocatedMemoryResourceOwned(std::size_t size, std::nullptr_t allocator) :
					allocator_	(allocator		),
					size_		(size			){}

		AllocatedMemoryResourceOwned(AllocatedMemoryResourceOwned &&other) :
					allocator_	(other.allocator_	),
					size_		(other.size_		),
					data_		(other.data_		){

			other.size_ = 0;
			other.data_ = nullptr;
		}

		~AllocatedMemoryResourceOwned(){
			MyAllocator::deallocate(allocator_, data_);
		}

		operator bool() const noexcept{
			return data_;
		}

		void *data() noexcept{
			return data_;
		}

		const void *data() const noexcept{
			return data_;
		}

		std::size_t size() const noexcept{
			return size_;
		}

	private:
		Allocator	allocator_;
		std::size_t	size_;
		void		*data_	= MyAllocator::allocate<char>(allocator_, size_);
	};



	template<typename T, class Allocator = std::nullptr_t>
	struct AllocatedMemoryResourceLinked{
		using value_type = void;
		using size_type  = std::size_t;

		AllocatedMemoryResourceLinked(std::size_t size, Allocator &allocator) :
					allocator_	(& allocator	),
					size_		(size		){}

		AllocatedMemoryResourceLinked(std::size_t size, std::nullptr_t) :
					allocator_	(nullptr	),
					size_		(size		){}

		AllocatedMemoryResourceLinked(AllocatedMemoryResourceLinked &other) :
					allocator_	(other.allocator_	),
					size_		(other.size_		),
					data_		(other.data_		){

			other.size_ = 0;
			other.data_ = nullptr;
		}

		~AllocatedMemoryResourceLinked(){
			MyAllocator::deallocate(allocator_, data_);
		}

		operator bool() const noexcept{
			return data_;
		}

		void *data() noexcept{
			return data_;
		}

		const void *data() const noexcept{
			return data_;
		}

		std::size_t size() const noexcept{
			return size_;
		}

	private:
		Allocator	*allocator_;
		std::size_t	size_;
		void		*data_	= MyAllocator::allocate(*allocator_, size_);
	};

} // namespace MyBuffer

#endif

