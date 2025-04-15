#ifndef MY_ALLOCATED_BUFFER
#define MY_ALLOCATED_BUFFER

#include "baseallocator.h"

namespace MyBuffer{
	namespace allocated_buffer_impl_{
		template<typename T, class Allocator>
		auto allocate(Allocator &allocator, std::size_t size){
			auto result = MyAllocator::wrapInSmartPtr(
					allocator,
					MyAllocator::allocate<T>(allocator, size)
			);

			if (result == nullptr)
				throw std::bad_alloc{};

			return result;
		}
	}



	template<typename T, class Allocator = std::nullptr_t>
	struct AllocatedBufferOwned{
		using value_type	= T;
		using size_type		= std::size_t;

		template<class ...Args>
		AllocatedBufferOwned(size_type const size, Args &&...args) :
					allocator_	(std::forward<Args>(args)...	),
					size_		(size				){}

		AllocatedBufferOwned(size_type const size, std::nullptr_t allocator) :
					allocator_	(allocator	),
					size_		(size		){}

		operator bool() const noexcept{
			return data_;
		}

		value_type *data() noexcept{
			return data_.get();
		}

		const value_type *data() const noexcept{
			return data_.get();
		}

		value_type &operator*() noexcept{
			return *data_;
		}

		value_type const &operator*() const noexcept{
			return *data_;
		}

		const value_type *operator->() const noexcept{
			return data_;
		}

		value_type *operator->() noexcept{
			return data_;
		}

		auto size() const noexcept{
			return size_;
		}

	private:
		using SmartPtrType = MyAllocator::SmartPtrType<value_type,Allocator>;

		Allocator	allocator_;
		size_type	size_;
		SmartPtrType	data_	= allocated_buffer_impl_::allocate<value_type>(allocator_, size_);
	};

	template<class Allocator = std::nullptr_t>
	using AllocatedByteBufferOwned = AllocatedBufferOwned<std::uint8_t, Allocator>;



	template<typename T, class Allocator = std::nullptr_t>
	struct AllocatedBufferLinked{
		using value_type	= T;
		using size_type		= std::size_t;

		AllocatedBufferLinked(size_type const size, Allocator &allocator) :
					allocator_	(& allocator	),
					size_		(size		){}

		AllocatedBufferLinked(size_type const size, std::nullptr_t allocator) :
					allocator_	(allocator	),
					size_		(size		){}

		operator bool() const noexcept{
			return data_;
		}

		value_type *data() noexcept{
			return data_.get();
		}

		const value_type *data() const noexcept{
			return data_.get();
		}

		value_type const &operator*() const noexcept{
			return *data_;
		}

		const value_type *operator->() const noexcept{
			return data_;
		}

		constexpr const value_type &operator[](size_t const index) const noexcept{
			return data_[index];
		}

		constexpr
		value_type &operator[](size_t const index) noexcept{
			return data_[index];
		}

		auto size() const noexcept{
			return size_;
		}

	private:
		using SmartPtrType = MyAllocator::SmartPtrType<value_type,Allocator>;

		Allocator	*allocator_;
		size_type	size_;
		SmartPtrType	data_	= allocated_buffer_impl_::allocate<value_type>(*allocator_, size_);
	};

	template<class Allocator = std::nullptr_t>
	using AllocatedByteBufferLinked = AllocatedBufferLinked<std::uint8_t, Allocator>;

} // namespace MyBuffer

#endif

