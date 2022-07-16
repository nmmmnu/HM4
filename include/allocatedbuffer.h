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
					allocator_	(std::forward<Args>(args)...	),
					size_		(size				){

			if (data_ == nullptr)
				throw std::bad_alloc{};
		}

		value_type *data() noexcept{
			return data_.get();
		}

		const value_type *data() const noexcept{
			return data_.get();
		}

		auto size() const noexcept{
			return size_;
		}

	private:
		using SmartPtrType = MyAllocator::SmartPtrType<value_type,Allocator>;

		static SmartPtrType allocate__(Allocator allocator, size_type size){
			return MyAllocator::wrapInSmartPtr(
					allocator,
					MyAllocator::allocate<value_type>(allocator, size)
			);
		}

	private:
		Allocator	allocator_;
		size_type	size_;
		SmartPtrType	data_	= allocate__(allocator_, size_);
	};

} // namespace MyBuffer

#endif

