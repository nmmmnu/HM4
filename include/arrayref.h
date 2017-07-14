#ifndef ARRAY_REF_H_
#define ARRAY_REF_H_

#include <cstdint>
#include <cassert>

template<class T>
class ArrayRef{
public:
	using size_type		= size_t;
	using value_type	= T;

	using Iterator		= const T*;

public:
	constexpr
	ArrayRef() = default;

	constexpr
	ArrayRef(const value_type *ptr, size_t const size) noexcept :
				ptr_(ptr),
				size_(size){}

	constexpr
	ArrayRef(const value_type *ptr, int const size) noexcept :
				ArrayRef(ptr, size > 0 ? (size_t) size : 0){}

	template<size_t N>
	constexpr
	ArrayRef(const value_type (&ptr)[N]) noexcept:
				ArrayRef(ptr, N){}

public:
	constexpr
	bool empty() const noexcept{
		return ptr_ == nullptr || size_ == 0;
	}

	constexpr
	size_type size() const noexcept{
		return size_;
	}

	constexpr
	size_t bytes() const noexcept{
		return size_ * sizeof(value_type);
	}

	constexpr
	const value_type &operator[](size_type const index) const noexcept{
		// preconditions
		assert(index < size_);
		assert(ptr_);
		// eo preconditions

		return ptr_[index];
	}

	constexpr
	const value_type *data() const noexcept{
		return ptr_;
	}

	constexpr
	Iterator begin() const noexcept{
		return ptr_;
	}

	constexpr
	Iterator end() const noexcept{
		return ptr_ + size_;
	}

private:
	const value_type	*ptr_	= nullptr;
	size_type		size_	= 0;
};

#endif


