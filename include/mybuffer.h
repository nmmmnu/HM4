#ifndef MY_BUFFER
#define MY_BUFFER

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	namespace impl_{

		template<typename T>
		constexpr bool is_byte =
			std::is_same_v<T, uint8_t> ||
			std::is_same_v<T, int8_t > ||
			std::is_same_v<T, char   >
		;

		template<typename T, typename Buffer>
		constexpr bool c_tor_1 =  std::is_same_v<T, typename Buffer::value_type>;

		template<typename T, typename Buffer>
		constexpr bool c_tor_2 = !std::is_same_v<T, typename Buffer::value_type> &&  is_byte<typename Buffer::value_type>;

		template<typename T, typename Buffer>
		constexpr bool c_tor_3 = !std::is_same_v<T, typename Buffer::value_type> && !is_byte<typename Buffer::value_type>;
	}

	template<typename T>
	struct BufferView{
		using value_type	= T;
		using size_type		= std::size_t;

		constexpr BufferView() = default;

		template<class Buffer, std::enable_if_t<impl_::c_tor_1<T, Buffer>, int> = 0>
		constexpr BufferView(Buffer &buffer) :
					BufferView( buffer.data(), buffer.size() ){}

		template<class Buffer, std::enable_if_t<impl_::c_tor_2<T, Buffer>, int> = 0>
		constexpr BufferView(Buffer &buffer) :
					BufferView( static_cast<void *>(buffer.data()), buffer.size() ){}

		template<class Buffer, std::enable_if_t<impl_::c_tor_3<T, Buffer>, int> = 0>
		constexpr BufferView(Buffer &buffer){
			static_assert(false, "Types must be the same or buffer must be from bytes");
		}

		constexpr BufferView(value_type *data, size_type size) :
					data_(data),
					size_(size){}

		constexpr BufferView(void *data, size_type size) :
					data_(reinterpret_cast<T *>(data)),
					size_(size / sizeof(T)){}

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

		constexpr auto size() const noexcept{
			return size_;
		}

	private:
		value_type	*data_	= nullptr;
		size_type	size_	= 0;
	};

	using ByteBufferView = BufferView<std::uint8_t>;



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

		constexpr static auto size() noexcept{
			return Size;
		}

	private:
		value_type	data_[Size] {};
	};

	template<std::size_t Size>
	using StaticByteBuffer = StaticBuffer<std::uint8_t, Size>;

} // namespace MyBuffer

#endif

