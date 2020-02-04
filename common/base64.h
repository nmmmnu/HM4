#ifndef MY_BASE64_H_
#define MY_BASE64_H_

#include <string_view>

std::string_view base64_decode(const char *data, size_t size, char *buffer);

inline std::string_view base64_decode(std::string_view const s, char *buffer){
	return base64_decode(s.data(), s.size(), buffer);
}

#endif

