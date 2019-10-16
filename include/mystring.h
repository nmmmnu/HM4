#ifndef MY_STRING_H_
#define MY_STRING_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

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

constexpr static auto hash(std::string_view const data) noexcept{
	return hash(data.data(), data.size() );
}



#endif

