#ifndef MY_ALLOCATED_BUFFER
#define MY_ALLOCATED_BUFFER

#include "baseallocator.h"
#include "mybuffer.h"

namespace MyBuffer{

	template<typename T, class Allocator = std::nullptr_t>
	struct AllocatedBuffer{
		using value_type	= T;
		using size_type		= std::size_t;

		template<class ...Args>
		AllocatedBuffer(size_type const size, Args &&...args) :
					allocator_( std::forward<Args>(args)...	),
					size_(size				){

			if (data_ == nullptr)
				throw std::bad_alloc{};
		}

		~AllocatedBuffer(){
			MyAllocator::deallocate(allocator_, data_);
		}

		value_type *data(){
			return data_;
		}

		const value_type *data() const{
			return data_;
		}

		auto size() const{
			return size_;
		}

	private:
		Allocator	allocator_;
		size_type	size_;
		value_type	 *data_	= MyAllocator::allocate<value_type>(allocator_, size_);
	};

} // namespace MyBuffer

#endif

