#ifndef MY_STATIC_BUFFER_
#define MY_STATIC_BUFFER_

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	template<std::size_t Size>
	struct StaticMemoryResource{
		using value_type	= void;
		using size_type		= std::size_t;

		constexpr operator bool() const noexcept{
			return true;
		}

		constexpr
		void *data() noexcept{
			return data_;
		}

		constexpr const void *data() const noexcept{
			return data_;
		}

		constexpr static std::size_t size() noexcept{
			return Size;
		}

	private:
		char data_[Size];
	};

} // namespace MyBuffer

#endif

