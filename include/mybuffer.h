#ifndef MY_BUFFER
#define MY_BUFFER

#include <cstdint>

namespace MyBuffer{

	template<typename T>
	struct LinkedBuffer{
		using value_type	= T;
		using size_type		= std::size_t;

		template<class Buffer>
		constexpr LinkedBuffer(Buffer &buffer) :
					LinkedBuffer(buffer.data(), buffer.size()){}

		constexpr LinkedBuffer(value_type *data, size_type size) :
					data_(data),
					size_(size){}

		constexpr
		value_type *data() noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

		constexpr auto size() const noexcept{
			return size_;
		}

	private:
		value_type	*data_;
		size_type	size_;
	};



	template<typename T, std::size_t Size>
	struct StaticBuffer{
		using value_type	= T;
		using size_type		= std::size_t;

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
		value_type	data_[Size] {};
	};

} // namespace MyBuffer

#endif

