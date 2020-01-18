#ifndef MY_STRING_H_
#define MY_STRING_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <array>	// buffer for to_chars

#ifdef NOT_HAVE_CHARCONV
#include <sstream>
#include <algorithm>	// copy_n
#else
#include <charconv>	// to_chars
#endif

inline int compare(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	auto sgn = [](auto a, auto b){
		return (a > b) - (a < b);
	};

	// First idea was lazy based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html

	if ( int const res = memcmp(s1, s2, std::min(size1, size2) ) )
		return res; // most likely exit

	// sgn helps convert size_t to int, without a branch
	return sgn(size1, size2);
}

constexpr bool equals(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	// Idea based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html
	return size1 == size2 && memcmp(s1, s2, size1) == 0;
}



template<typename... Args>
std::string concatenate(Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	// super cheap concatenation,
	// with single allocation

	size_t const reserve_size = (std::string_view{ args }.size() + ...);

	std::string s;

	s.reserve(reserve_size);

	(s.append(std::forward<Args>(args)), ...);

	return s;
}

template<typename... Args>
std::string const &concatenate(std::string &s, Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	// super cheap concatenation,
	// sometimes without allocation

	size_t const reserve_size = (std::string_view{ args }.size() + ...);

	s.clear();

	// reserve() will shrink capacity
	if (reserve_size > s.capacity())
		s.reserve(reserve_size);

	(s.append(std::forward<Args>(args)), ...);

	return s;
}



constexpr uint32_t hash(const char* data, size_t const size) noexcept{
	uint32_t hash = 5381;

	for(const char *c = data; c < data + size; ++c)
		hash = ((hash << 5) + hash) + (unsigned char) *c;

	return hash;
}

constexpr auto hash(std::string_view const data) noexcept{
	return hash(data.data(), data.size() );
}



using to_string_buffer_t = std::array<char, 64>;



#ifdef NOT_HAVE_CHARCONV
	// Based on ston_safe.h

	/*
	template<typename T>
	std::string to_string(T const value){
		static_assert(std::is_integral_v<T>, "T must be integral");

		std::stringstream ss;
		ss << value;
		return ss.str();
	}
	*/

	template<typename T>
	std::string_view to_string(T const value, to_string_buffer_t &buffer){
		static_assert(std::is_integral_v<T>, "T must be integral");

		auto const s    = std::to_string(value);
		auto const size = std::min(s.size(), buffer.size());

		// copy...
		std::copy_n(std::begin(s), size, std::begin(buffer));

		return std::string_view{ buffer.data(), size };
	}

	template<typename T>
	T from_string(std::string_view const s, T const default_value = T{0}){
		static_assert(std::is_integral_v<T>, "T must be integral");

		if (s.empty())
			return default_value;

		T u{0};
		std::istringstream ss(s.data());
		ss >> u;
		return u;
	}

#else
	// Modern faster <charconv>

	template<typename T>
	std::string_view to_string(T const value, to_string_buffer_t &buffer){
		static_assert(std::is_integral_v<T>, "T must be integral");

		auto [p, ec] = std::to_chars(std::begin(buffer), std::end(buffer), value);

		if (ec != std::errc())
			return "0";

		auto c = [](auto a){
			return static_cast<std::string_view::size_type>(a);
		};

		return std::string_view{ buffer.data(), c(p - buffer.data()) };
	}

	template<typename T>
	T from_string(std::string_view const s, T const default_value = T{0}){
		static_assert(std::is_integral_v<T>, "T must be integral");

		T value;

		auto [p, ec] = std::from_chars(std::begin(s), std::end(s), value);

		if (ec != std::errc())
			return default_value;

		return value;
	}

#endif



#endif

