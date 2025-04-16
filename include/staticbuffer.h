#ifndef MY_STATIC_BUFFER_
#define MY_STATIC_BUFFER_

#include <cstdint>

namespace MyBuffer{

	template<std::size_t Size>
	struct StaticBufferResource{
		using value_type = char;
		using size_type  = std::size_t;

		constexpr operator bool() noexcept{
			return true;
		}

		constexpr
		value_type *data() noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

		constexpr static size_type size() noexcept{
			return Size;
		}

	private:
		value_type data_[Size] {};
	};

} // namespace MyBuffer

#endif

