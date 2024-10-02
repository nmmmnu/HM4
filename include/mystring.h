#ifndef MY_STRING_H_
#define MY_STRING_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <array>	// buffer for to_chars
#include <algorithm>	// clamp, copy_n for NOT_HAVE_CHARCONV

#ifdef NOT_HAVE_CHARCONV
#include <sstream>
#else
#include <charconv>	// to_chars
#endif

inline int compare(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	auto sgn = [](auto a, auto b){
		return (a > b) - (a < b);
	};

	// trick from gperf by comparing first characters
	// checking sizes is a must

	if (size1 && size2 && *s1 != *s2)
		return sgn(*s1, *s2);

	// First idea was lazy based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html

	if ( int const res = memcmp(s1, s2, std::min(size1, size2) ) )
		return res; // most likely exit

	// sgn helps convert size_t to int, without a branch
	return sgn(size1, size2);
}

constexpr bool equals(const char *s1, size_t const size1, const char *s2, size_t const size2) noexcept{
	// trick from gperf by comparing first characters

	// Idea based on LLVM::StringRef
	// http://llvm.org/docs/doxygen/html/StringRef_8h_source.html

	// if size == 0, *s1 == *s2 is invalid...
	return size1 == size2 && memcmp(s1, s2, size1) == 0;
}

constexpr bool same_prefix(std::string_view const prefix, std::string_view const s) noexcept{
	auto const size1 = prefix.size();
	auto const size2 = s.size();

	return size1 <= size2 && equals(prefix.data(), size1, s.data(), size1);
}

inline std::string_view after_prefix(size_t const prefix_size, std::string_view const s) noexcept{
	if (prefix_size <= s.size())
		return s.substr(prefix_size);
	else
		return s;
}

inline std::string_view after_prefix(std::string_view const prefix, std::string_view const s) noexcept{
	return after_prefix(prefix.size(), s);
}



template<typename... Args>
std::string_view concatenateRawBuffer_(char *buffer, Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	size_t pos = 0;

	auto append = [&pos, &buffer](std::string_view x){
		memcpy(buffer + pos, x.data(), x.size());
		pos += x.size();
	};

	(append(std::forward<Args>(args)), ...);

	return std::string_view{ buffer, pos };
}



template<typename... Args>
size_t concatenateBufferSize(Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	return (std::string_view{ args }.size() + ...);
}



template<size_t N, typename... Args>
std::string_view concatenateBuffer(std::array<char, N> &buffer, Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	size_t const reserve_size = concatenateBufferSize(std::forward<Args>(args)...);

	if (reserve_size > buffer.size())
		return "";

	return concatenateRawBuffer_(buffer.data(), std::forward<Args>(args)...);
}



template<typename... Args>
std::string_view concatenateBuffer(std::string &buffer, Args &&... args){
	static_assert((std::is_constructible_v<std::string_view, Args> && ...));

	// super cheap concatenation,
	// sometimes without allocation

	size_t const reserve_size = concatenateBufferSize(std::forward<Args>(args)...);

	buffer.clear();

	// reserve() will shrink capacity
	if (reserve_size > buffer.capacity())
		buffer.reserve(reserve_size);

	(buffer.append(std::forward<Args>(args)), ...);

	return buffer;
}



template<typename... Args>
std::string concatenateString(Args &&... args){
	std::string s;

	concatenateBuffer(s, std::forward<Args>(args)...);

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



constexpr size_t to_string_buffer_t_size = 32;	// largest uint64_t is 20 digits.
						// largest double is less 32.
using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;



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

	template<size_t N, typename T>
	std::string_view to_string(T const value, std::array<char, N> &buffer){
		static_assert(N >= to_string_buffer_t_size,	"use to_string_buffer_t instead");
		static_assert(std::is_integral_v<T>,		"T must be integral");

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

	template<size_t N, typename T>
	std::string_view to_string(T const value, std::array<char, N> &buffer){
		static_assert(N >= to_string_buffer_t_size,	"use to_string_buffer_t instead");
		static_assert(std::is_integral_v<T>,		"T must be integral");

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



template<typename T>
auto myClamp(std::string_view p, uint64_t min, uint64_t max){
	// I already have trait for this,
	// but I do not want to include it here.

        static_assert(
            std::is_same_v<T, uint8_t > ||
            std::is_same_v<T, uint16_t> ||
            std::is_same_v<T, uint32_t> ||
            std::is_same_v<T, uint64_t>
        );

	// Using uint64_t from the user, allow more user-friendly behavour.
	// Suppose he / she enters 1'000'000'000.
	// Because this value is great than max uint32_t,
	// The converted value will go to 0, then to MIN.

	auto const a = from_string<uint64_t>(p);

	return static_cast<T>(
		std::clamp<uint64_t>(a, min, max)
	);
};



#endif

