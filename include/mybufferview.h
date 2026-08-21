#ifndef MY_BUFFER
#define MY_BUFFER

#include <cstdint>
#include <type_traits>

namespace MyBuffer{

	struct from_bytes{};

	template<typename T>
	struct BufferView{
		using value_type	= T;
		using size_type		= std::size_t;

	private:
		value_type	*data_	= nullptr;
		size_type	size_	= 0;

	private:
		template<typename U>
		constexpr static bool is_byte_v__(){
			return	std::is_same_v<std::remove_cv_t<U>,          char>	||
				std::is_same_v<std::remove_cv_t<U>, signed   char>	||
				std::is_same_v<std::remove_cv_t<U>, unsigned char>	||
				std::is_same_v<std::remove_cv_t<U>, void>
			;
		}

	public:
		constexpr BufferView()	= default;

		constexpr BufferView(value_type *data, size_type size) :
					data_(data),
					size_(size){}

		template<typename U,
				std::enable_if_t<
					// T is const		and
					// T is same as U	and
					// U may not be const
					std::is_const_v<T> &&
					std::is_same_v<
						std::remove_cv_t<T>,
						std::remove_cv_t<U>
					>,
				int> = 0
		>
		constexpr BufferView(BufferView<U> const &buffer) :
					BufferView( buffer.data(), buffer.size() ){}

	public:
		template<typename U,
				std::enable_if_t<
					// T is not const	and
					// U is not const	and
					// U is char
					!std::is_const_v<T> &&
					!std::is_const_v<U> &&
					 is_byte_v__<U>(),
				int> = 0
		>
		constexpr BufferView(U *data, size_type size, from_bytes) :
					data_(reinterpret_cast<value_type *>(data)),
					size_(size / sizeof(T)){}

		template<typename U,
				std::enable_if_t<
					// T is const		and
					// U may be non const	and
					// U is char
					std::is_const_v<T> &&
					is_byte_v__<U>(),
				int> = 0
		>
		constexpr BufferView(U *data, size_type size, from_bytes) :
					data_(reinterpret_cast<const value_type *>(data)),
					size_(size / sizeof(T)){}

	public:
		template<typename Container,
			std::enable_if_t<
					std::is_constructible_v<BufferView,
						decltype(std::declval<Container &>().data()),
						decltype(std::declval<Container &>().size())
					>,
				int> = 0
		>
		constexpr BufferView(Container &container) :
					BufferView(container.data(), container.size()){}

		template<typename Container,
			std::enable_if_t<
					std::is_constructible_v<BufferView,
						decltype(std::declval<Container &>().data()),
						decltype(std::declval<Container &>().size()),
						from_bytes
					>,
				int> = 0
		>
         	constexpr BufferView(Container &container, from_bytes) :
					BufferView(container.data(), container.size(), from_bytes{}){}

	public:
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
	};



	using ByteBufferView = BufferView<char>;



	namespace test_{

		template <typename T>
		struct CB{
			T *data()		{ return nullptr;	}
			std::size_t size()	{ return 0;		}
		};

		// from BufferView
		static_assert( std::is_constructible_v<BufferView<int      >,       BufferView<int       > >);
		static_assert(!std::is_constructible_v<BufferView<int      >,       BufferView<int  const> >);

		static_assert( std::is_constructible_v<BufferView<int const>,       BufferView<int       > >);
		static_assert( std::is_constructible_v<BufferView<int const>,       BufferView<int  const> >);

		static_assert(!std::is_constructible_v<BufferView<int      >,       BufferView<long const> >);

		// from char
		static_assert( std::is_constructible_v<BufferView<int      >,       char *, std::size_t, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>,       char *, std::size_t, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, const char *, std::size_t, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, const char *, std::size_t, from_bytes>);

		static_assert( std::is_constructible_v<BufferView<int      >, CB<      char> &, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, CB<      char> &, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, CB<const char> &, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, CB<const char> &, from_bytes>);

		// from void
		static_assert( std::is_constructible_v<BufferView<int      >,       void *, std::size_t, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>,       void *, std::size_t, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, const void *, std::size_t, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, const void *, std::size_t, from_bytes>);

		static_assert( std::is_constructible_v<BufferView<int      >, CB<      void> &, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, CB<      void> &, from_bytes>);
		static_assert( std::is_constructible_v<BufferView<int const>, CB<const void> &, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, CB<const void> &, from_bytes>);

		// from random type
		static_assert(!std::is_constructible_v<BufferView<int      >,       long *, std::size_t, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int const>,       long *, std::size_t, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int const>, const long *, std::size_t, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, const long *, std::size_t, from_bytes>);

		static_assert(!std::is_constructible_v<BufferView<int      >, CB<      long> &, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int const>, CB<      long> &, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int const>, CB<const long> &, from_bytes>);
		static_assert(!std::is_constructible_v<BufferView<int      >, CB<const long> &, from_bytes>);

	} // namespace test_

} // namespace MyBuffer

#endif

