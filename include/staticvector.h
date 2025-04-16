#ifndef MY_FIXED_VECTOR_H_
#define MY_FIXED_VECTOR_H_

#include "bufferedvector.h"
#include "mybuffer.h"

namespace static_vector_impl_{
	template<typename T, std::size_t Size>
	struct StaticBuffer{
		using value_type        = T;
		using size_type         = std::size_t;

		constexpr
		value_type *data() noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

		constexpr static auto size() noexcept{
			return Size;
		}

	private:
		value_type      data_[Size] {};
	};
} // namespace static_vector_impl_

template<typename T, std::size_t Size>
using StaticVector = BufferedVector<T, static_vector_impl_::StaticBuffer<T, Size> >;

static_assert(std::is_trivially_destructible_v<StaticVector<int, 100> >, "StaticVector must be POD-like type");

#endif

