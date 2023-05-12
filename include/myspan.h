#ifndef MY_SPAN_VECTOR_H_
#define MY_SPAN_VECTOR_H_

#include <initializer_list>
#include <string_view>

enum class MySpanConstructor{
	NORMAL		,
	EXPLICIT
};

template<typename T, MySpanConstructor = MySpanConstructor::NORMAL>
class MySpan{
public:
	// TYPES

	using value_type	= T;
	using size_type		= std::size_t;
	using difference_type	= std::ptrdiff_t;

	using reference		= const T&;
	using const_reference	= const T&;

	using pointer		= const T*;
	using const_pointer	= const T*;

	using iterator		= const T*;
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
		// see [1] behavior is undefined
		return data()[index];
	}

	constexpr const value_type &operator[](size_type const index) const noexcept{
		// see [1] behavior is undefined
		return data()[index];
	}

	// FRONT

	constexpr value_type &front() noexcept{
		// see [1] behavior is undefined
		return data()[0];
	}

	constexpr const value_type &front() const noexcept{
		// see [1] behavior is undefined
		return data()[0];
	}

	// BACK

	constexpr value_type &back() noexcept{
		// see [1] behavior is undefined
		return data()[size_ - 1];
	}

	constexpr const value_type &back() const noexcept{
		// see [1] behavior is undefined
		return data()[size_ - 1];
	}

private:
	constexpr
	void validateIndex_(size_type const index) const{
		if (index >= size_){
			throw std::out_of_range("Out of Range");
		}
	}
};



template<typename T>
struct MySpan<T, MySpanConstructor::EXPLICIT> : public MySpan<T, MySpanConstructor::NORMAL>{
	using MySpan<T, MySpanConstructor::NORMAL>::MySpan;

	template<class Container>
	explicit
	constexpr MySpan(const Container &v) : MySpan<T, MySpanConstructor::NORMAL>(v){}
};



template<typename T, MySpanConstructor SC = MySpanConstructor::NORMAL>
auto blobAsMySpan(void *ptr, size_t size){
	return MySpan<T, SC>{
		reinterpret_cast<T *>(ptr),
		size / sizeof(T)
	};
}

template<typename T, MySpanConstructor SC = MySpanConstructor::NORMAL>
auto blobAsMySpan(const void *ptr, size_t size){
	return MySpan<const T, SC>{
		reinterpret_cast<T *>(ptr),
		size / sizeof(T)
	};
}

template<typename T, MySpanConstructor SC = MySpanConstructor::NORMAL>
auto blobAsMySpan(std::string_view s){
	return blobAsMySpan<T, SC>(s.data(), s.size());
}



#endif

