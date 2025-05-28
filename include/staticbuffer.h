#ifndef MY_STATIC_BUFFER_
#define MY_STATIC_BUFFER_

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	template<typename T, std::size_t Size>
	struct StaticBuffer{
		using value_type	= T;
		using size_type		= std::size_t;

		constexpr operator bool() const noexcept{
			return true;
		}

		constexpr
		value_type *data() noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

		constexpr
		value_type &operator*() noexcept{
			return *data_;
		}

		constexpr value_type const &operator*() const noexcept{
			return *data_;
		}

		constexpr const value_type *operator->() const noexcept{
			return data_;
		}

		constexpr
		value_type *operator->() noexcept{
			return data_;
		}

		constexpr const value_type &operator[](size_t const index) const noexcept{
			return data_[index];
		}

		constexpr
		value_type &operator[](size_t const index) noexcept{
			return data_[index];
		}

		constexpr static size_type size() noexcept{
			return Size;
		}

	private:
		value_type data_[Size] {};
	};

	template<std::size_t Size>
	using ByteStaticBuffer		= StaticBuffer<char, Size>;



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

