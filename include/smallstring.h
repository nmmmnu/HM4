#ifndef MY_SMALL_STRING_H_
#define MY_SMALL_STRING_H_

#include <stringref.h>

#include <cassert>

template <size_t BYTES>
class SmallString{
	static_assert(BYTES >= 1 && BYTES <= 8, "BYTES must be between 1 and 8");

private:
	constexpr static size_t SIZE = 8 * BYTES;

public:
	// CONSTRUCTORS

	SmallString() noexcept{
		clear_();
	}

	SmallString(const char *data) noexcept{
		if (data)
			strncpy(data_, data, SIZE);
		else
			clear_();
	}

	SmallString(const char *data, size_t const size) noexcept{
		if (data)
			copy_(data, size);
		else
			clear_();
	}

	SmallString(const StringRef &sr) noexcept{
		copy_(sr.data(), sr.size());
	}

private:
	void copy_(const char *data, size_t const size) noexcept{
		if (size >= SIZE){
			memcpy(data_, data, SIZE);
		}else{
			memcpy(data_, data, size);
			memset(data_ + size, 0, SIZE - size);
		}
	}

	void clear_() noexcept{
		memset(data_, 0, SIZE);
	}

public:
	// INFO

	constexpr static size_t capacity() noexcept{
		return SIZE;
	}

	// DATA MEMBERS AND CASTS

	constexpr const char *data() const noexcept{
		return data_;
	}

	constexpr const char *c_str() const noexcept{
		return data();
	}

	size_t size() const noexcept{
		return strnlen(data_, SIZE);
	}

	size_t length() const noexcept{
		return size();
	}

	constexpr bool empty() const noexcept{
		return data_[0] == 0;
	}

	// ITERATORS

	constexpr const char &operator [] (size_t const index) const noexcept{
		assert(index < SIZE);
		return data_[index];
	}

	constexpr const char *begin() const noexcept{
		return data_;
	}

	const char *end() const noexcept{
		return data_ + size();
	}

	// COMPARES

	int compare(const StringRef &sr) const noexcept{
		return compare(sr.data(), sr.size());
	}

	int compare(const char *data, size_t const size) const noexcept{
		return compare_(data, min__(size));
	}

	int compare(const char *data) const noexcept{
		return compare_(data, SIZE);
	}

	// EQUALS

	bool equals(const StringRef &sr) const noexcept{
		return equals(sr.data(), sr.size());
	}

	bool equals(const char *data, size_t size) const noexcept{
		return equals_(data, min__(size));
	}

	bool equals(const char *data) const noexcept{
		return equals_(data, SIZE);
	}

	// OPERATORS NOT INCLUDED

private:
	// COMPARES / EQUALS HELPERS

	int compare_(const char *data, size_t const size) const noexcept{
		// if data_ is less SIZE, it is null terminated
		return strncmp(data_, data, size);
	}

	bool equals_(const char *data, size_t const size) const noexcept{
		// if data_ is less SIZE, it is null terminated
		return strncmp(data_, data, size) == 0;
	}

private:
	// HELPERS

	constexpr static size_t min__(size_t const size) noexcept{
		return size < SIZE ? size : SIZE;
	}

private:
	char data_[SIZE];
};

// ==================================

template <size_t BYTES>
inline std::ostream& operator << (std::ostream& os, const SmallString<BYTES> &sr){
	// cast because of clang
	//return os.write(sr.data(), static_cast<std::streamsize>( sr.size() ));
	// almost the same, but std::setw() works
	return __ostream_insert(os, sr.data(), static_cast<std::streamsize>( sr.size() ));
}

#endif

