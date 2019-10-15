#ifndef MY_SMALL_STRING_H_
#define MY_SMALL_STRING_H_

#include "stringref.h"

#include <cassert>
#include <type_traits>

template <size_t BYTES>
class SmallString{
	static_assert(BYTES >= 1 && BYTES <= 64, "BYTES must be between 1 and 64");

private:
	constexpr static size_t SIZE = BYTES;

public:
	// CONSTRUCTORS

	SmallString() noexcept{
		clear_();
	}

	SmallString(const char *data) noexcept{
		if (data){
			// https://stackoverflow.com/questions/50198319/gcc-8-wstringop-truncation-what-is-the-good-practice
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wstringop-truncation"
			strncpy(data_, data, SIZE);
			#pragma GCC diagnostic pop
		}else
			clear_();
	}

	SmallString(const char *data, size_t const size) noexcept{
		if (data)
			copyNotNull_(data, size);
		else
			clear_();
	}

	SmallString(const StringRef &sr) noexcept{
		copyNotNull_(sr.data(), sr.size());
	}

private:
	void copyNotNull_(const char *data, size_t const size) noexcept{
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
		return strnlen__(data_);
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
		assert(data);
		return compare_(data, min__(size));
	}

	int compare(const char *data) const noexcept{
		return compare_(data, strnlen__(data));
	}

	// EQUALS

	bool equals(const StringRef &sr) const noexcept{
		return equals(sr.data(), sr.size());
	}

	bool equals(const char *data, size_t size) const noexcept{
		assert(data);
		return equals_(data, min__(size));
	}

	bool equals(const char *data) const noexcept{
		assert(data);
		return equals_(data, SIZE);
	}

	// OPERATORS NOT INCLUDED
public:
	// STATIC HELPER

	static int compare(const char *smalldata, const char *data, size_t const size) noexcept{
		auto const smallsize = strnlen__(smalldata);
		auto const fixedsize = min__(size);

		return ::compare(smalldata, smallsize, data, fixedsize);
	}

	static bool equals(const char *smalldata, const char *data, size_t const size) noexcept{
		auto const smallsize = strnlen__(smalldata);
		auto const fixedsize = min__(size);

		return ::equals(smalldata, smallsize, data, fixedsize);
	}

private:
	// COMPARES / EQUALS HELPERS

	int compare_(const char *data, size_t const size) const noexcept{
		return compare(data_, data, size);
	}

	bool equals_(const char *data, size_t const size) const noexcept{
		return equals(data_, data, size);
	}

private:
	// HELPERS

	constexpr static size_t min__(size_t const size) noexcept{
		return size < SIZE ? size : SIZE;
	}

	static size_t strnlen__(const char *s) noexcept{
		return strnlen(s, SIZE);
	}

private:
	char data_[SIZE];
};

static_assert( std::is_trivially_copyable<SmallString<8> >::value, "SmallString is not trivially copyable" );

// ==================================

template <size_t BYTES>
inline std::ostream& operator << (std::ostream& os, const SmallString<BYTES> &ss){
	os << StringRef{ ss.data(), ss.size() };

	return os;
}

template <size_t BYTES>
inline int compareFull(const StringRef &s, const SmallString<BYTES> &ss, const StringRef &ls) noexcept{
	int const r = ss.compare(s);

	if (r || s.size() < BYTES){
		// if s.size() == BYTES,
		// this does not mean that strings are equal,
		// because ls might be longer
		return r;
	}

	// comparing substring seems to be more expencive
	return ls.compare(s);
}

template <size_t BYTES>
inline bool equalsFull(const StringRef &s, const SmallString<BYTES> &ss, const StringRef &ls) noexcept{
	(void) s;
	(void) ss;
	(void) ls;
	assert(false); // todo
}

// ==================================

using SmallString8  = SmallString<8>;
using SmallString16 = SmallString<16>;

#endif

