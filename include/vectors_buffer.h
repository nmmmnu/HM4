#ifndef MY_VECTORS_BUFFER_H_
#define MY_VECTORS_BUFFER_H_

namespace MyVectors{

	template<typename T>
	struct TVector{
		using value_type	= T;
		using size_type		= size_t;

		constexpr TVector() = default;
		constexpr TVector(value_type *data, size_type size) : data_(data), size_(size){}
		template<typename U,
				std::enable_if_t<
					// T is const		and
					// T is same as U	and
					// U may not be const
					std::is_const_v<T> &&
					std::is_same_v<T, U const>,
				int> = 0
		>
		constexpr TVector(TVector<U> const &buffer) :
					TVector( buffer.data(), buffer.size() ){}

		constexpr const value_type &operator[](size_type const index) const noexcept{
			return data_[index];
		}

		constexpr value_type &operator[](size_type const index) noexcept{
			return data_[index];
		}

		constexpr size_type size() const noexcept{
			return size_;
		}
		constexpr value_type *data() noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

	private:
		value_type	*data_	= nullptr;
		size_type	size_	= 0;
	};

	template<typename T>
	using CTVector = TVector<T const>;

	using FVector  = TVector<float>;

	using CFVector = CTVector<float const>;

} // namespace MyVectors

#endif

