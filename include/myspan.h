#ifndef MY_SPAN_VECTOR_H_
#define MY_SPAN_VECTOR_H_

#include <string_view>
#include <cassert>

enum class MySpanConstructor{
	NORMAL			,
	EXPLICIT
};

template<typename T, MySpanConstructor = MySpanConstructor::NORMAL, bool RangeCheck = false>
class MySpan{
public:
	// TYPES

	using value_type	= T;
	using size_type		= std::size_t;
	using difference_type	= std::ptrdiff_t;

	using reference		=       T&;
	using const_reference	= const T&;

	using pointer		=       T*;
	using const_pointer	= const T*;

	using iterator		=       T*;
	using const_iterator	= const T*;

private:
	T		*data_;
	size_type	size_;

public:
	constexpr MySpan(T *data, size_type size) : data_(data), size_(size){}

	template<class Container>
	constexpr MySpan(const Container &v) : MySpan(v.data(), v.size()){}

	// COMPARISSON

	constexpr bool operator==(const MySpan &other) const noexcept{
		if (size_ != other.size_)
			return false;

		auto first = other.begin();
		auto last  = other.end();
		auto me    = begin();

		for(; first != last; ++first, ++me){
			if ( ! (*first == *me) )
				return false;
		}

		return true;
	}

	constexpr bool operator!=(const MySpan &other) const noexcept{
		return ! operator==(other);
	}

	// ITERATORS

	constexpr iterator begin() noexcept{
		return data();
	}

	constexpr iterator end() noexcept{
		return data() + size();
	}

	// CONST ITERATORS

	constexpr const_iterator begin() const noexcept{
		return data();
	}

	constexpr const_iterator end() const noexcept{
		return data() + size();
	}

	// C++11 CONST ITERATORS

	constexpr const_iterator cbegin() const noexcept{
		return begin();
	}

	constexpr const_iterator cend() const noexcept{
		return end();
	}

	// Size

	constexpr size_type size() const noexcept{
		return size_;
	}

	constexpr bool empty() const noexcept{
		return size() == 0;
	}

	// DATA

	constexpr value_type *data() noexcept{
		return data_;
	}

	constexpr const value_type *data() const noexcept{
		return data_;
	}

	// ACCESS WITH RANGE CHECK

	constexpr value_type &at(size_type const index){
		validateIndex_(index);
		return data()[index];
	}

	constexpr const value_type &at(size_type const index) const{
		validateIndex_(index);
		return data()[index];
	}

	// ACCESS DIRECTLY

	constexpr value_type &operator[](size_type const index) noexcept{
		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

	constexpr const value_type &operator[](size_type const index) const noexcept{
		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

	// FRONT

	constexpr value_type &front() noexcept{
		size_type const index = 0;

		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

	constexpr const value_type &front() const noexcept{
		size_type const index = 0;

		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

	// BACK

	constexpr value_type &back() noexcept{
		size_type const index = size_ - 1;

		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

	constexpr const value_type &back() const noexcept{
		size_type const index = size_ - 1;

		validateIndexCheck_(index);

		// see [1] behavior is undefined
		return data()[index];
	}

private:
	constexpr
	bool validateIx_(size_type const index) const{
		return size_ > 0 && index < size_;
	}

	constexpr
	void validateIndex_(size_type const index) const{
		if (!validateIx_(index)){
			throw std::out_of_range("Out of Range");
		}
	}

	constexpr
	void validateIndexCheck_(size_type const index) const{
		if constexpr(RangeCheck){
			assert(validateIx_(index));
		}
	}
};



template<typename T, bool RangeCheck>
struct MySpan<T, MySpanConstructor::EXPLICIT, RangeCheck> : public MySpan<T, MySpanConstructor::NORMAL, RangeCheck>{
	using MySpan<T, MySpanConstructor::NORMAL>::MySpan;

	template<class Container>
	explicit
	constexpr MySpan(const Container &v) : MySpan<T, MySpanConstructor::NORMAL>(v){}
};



template<typename T, bool RangeCheck = false>
auto blobAsMySpan(void *ptr, size_t size){
	return MySpan<T, MySpanConstructor::NORMAL>{
		static_cast<T *>(ptr),
		size / sizeof(T)
	};
}

template<typename T_, bool RangeCheck = false>
auto blobAsMySpan(const void *ptr, size_t size){
	using T = const T_;

	return MySpan<T, MySpanConstructor::NORMAL>{
		static_cast<T *>(ptr),
		size / sizeof(T)
	};
}

template<typename T, bool RangeCheck = false>
auto blobAsMySpan(std::string_view s){
	return blobAsMySpan<T, RangeCheck>(s.data(), s.size());
}



#endif

