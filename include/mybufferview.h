#ifndef MY_BUFFER
#define MY_BUFFER

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	template<typename T>
	struct BufferView{
		using value_type	= T;
		using size_type		= std::size_t;



		constexpr BufferView() = default;



		template<typename U,
				std::enable_if_t<
					// T is const		and
					// T is same as U	and
					// U may not be const
					std::is_const_v<T> &&
					std::is_same_v<T, U const>,
				int> = 0
		>
		constexpr BufferView(BufferView<U> const &buffer) :
					BufferView( buffer.data(), buffer.size() ){}



		template<typename Buffer,
				std::enable_if_t<
					// Buffer::value_type is byte
					std::is_same_v<typename Buffer::value_type, uint8_t> ||
					std::is_same_v<typename Buffer::value_type, int8_t > ||
					std::is_same_v<typename Buffer::value_type, char   > ||
					std::is_same_v<typename Buffer::value_type, void   >,
				int> = 0
		>
		constexpr BufferView(Buffer &buffer) :
					BufferView( static_cast<void *>(buffer.data()), buffer.size() ){}



		template<typename Buffer,
				std::enable_if_t<
					// Buffer::value_type is byte const
					std::is_same_v<typename Buffer::value_type, uint8_t const> ||
					std::is_same_v<typename Buffer::value_type, int8_t  const> ||
					std::is_same_v<typename Buffer::value_type, char    const> ||
					std::is_same_v<typename Buffer::value_type, void    const>,
				int> = 0
		>
		constexpr BufferView(Buffer const &buffer) :
					BufferView( static_cast<const void *>(buffer.data()), buffer.size() ){}



		constexpr BufferView(value_type *data, size_type size) :
					data_(data),
					size_(size){}

		constexpr BufferView(void *data, size_type size) :
					data_(reinterpret_cast<T *>(data)),
					size_(size / sizeof(T)){}

		constexpr BufferView(const void *data, size_type size) :
					data_(reinterpret_cast<const T *>(data)),
					size_(size / sizeof(T)){}



		constexpr operator bool() const noexcept{
			return data_;
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

		constexpr const value_type &operator[](size_type const index) const noexcept{
			return data_[index];
		}

		constexpr
		value_type &operator[](size_type const index) noexcept{
			return data_[index];
		}

		constexpr size_type size() const noexcept{
			return size_;
		}

	private:
		value_type	*data_	= nullptr;
		size_type	size_	= 0;
	};

	using ByteBufferView = BufferView<char>;

} // namespace MyBuffer

#endif

