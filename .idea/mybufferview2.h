#ifndef MY_BUFFER
#define MY_BUFFER

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	template<typename T>
	struct BufferView{
		using value_type	= T;
		using size_type		= std::size_t;

		using value_type_nc	= std::remove_cv_t<T>;

		constexpr BufferView() = default;



		template<typename U, std::enable_if_t<std::is_same_v<std::remove_cv_t<U>, std::remove_cv_t<T> > && std::is_const_v<T>, int> = 0>
		constexpr BufferView(const BufferView<U>& other) : BufferView(other.data(), other.size()) {}



		constexpr BufferView(BufferView<uint8_t> &buffer) : BufferView( static_cast<void *>(buffer.data()), buffer.size() ){}
		constexpr BufferView(BufferView<int8_t > &buffer) : BufferView( static_cast<void *>(buffer.data()), buffer.size() ){}
		constexpr BufferView(BufferView<char   > &buffer) : BufferView( static_cast<void *>(buffer.data()), buffer.size() ){}



		constexpr BufferView(BufferView<uint8_t const> &buffer) : BufferView( static_cast<const void *>(buffer.data()), buffer.size() ){}
		constexpr BufferView(BufferView<int8_t  const> &buffer) : BufferView( static_cast<const void *>(buffer.data()), buffer.size() ){}
		constexpr BufferView(BufferView<char    const> &buffer) : BufferView( static_cast<const void *>(buffer.data()), buffer.size() ){}



		constexpr BufferView(      value_type_nc *data, size_type size) : data_(data), size_(size){}
		constexpr BufferView(const value_type_nc *data, size_type size) : data_(data), size_(size){}



		constexpr BufferView(      void *data, size_type size) : data_(reinterpret_cast<      T *>(data)), size_(size / sizeof(T)){}
		constexpr BufferView(const void *data, size_type size) : data_(reinterpret_cast<const T *>(data)), size_(size / sizeof(T)){}

		constexpr operator BufferView<value_type_nc const>() const{
			return BufferView<value_type_nc const>(data_, size_);
		}

		constexpr operator bool() const noexcept{
			return data_;
		}

		constexpr BufferView<T const> asConst() const{
			return BufferView<T const>{ data(), size() };
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

