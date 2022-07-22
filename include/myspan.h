#ifndef MY_SPAN_VECTOR_H_
#define MY_SPAN_VECTOR_H_

#include <initializer_list>

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
	const T		*data_;
	size_type	size_;

public:
	constexpr MySpan(const T *data, size_type size) : data_(data), size_(size){}

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

	constexpr const value_type *data() const noexcept{
		return data_;
	}

	// ACCESS WITH RANGE CHECK

	constexpr const value_type &at(size_type const index) const{
		validateIndex_(index);
		return data()[index];
	}

	// ACCESS DIRECTLY

	constexpr const value_type &operator[](size_type const index) const noexcept{
		// see [1] behavior is undefined
		return data()[index];
	}

	// FRONT

	constexpr const value_type &front() const noexcept{
		// see [1] behavior is undefined
		return data()[0];
	}

	// BACK

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



#endif

