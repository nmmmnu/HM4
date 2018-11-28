#ifndef MY_TYPE_NAME_H_
#define MY_TYPE_NAME_H_

#include <cstdint>

template<typename T>
constexpr const char *my_typename = "unknown";

template<> constexpr const char *my_typename<int8_t	> = "int8_t"	;
template<> constexpr const char *my_typename<int16_t	> = "int16_t"	;
template<> constexpr const char *my_typename<int32_t	> = "int32_t"	;
template<> constexpr const char *my_typename<int64_t	> = "int64_t"	;

template<> constexpr const char *my_typename<uint8_t	> = "uint8_t"	;
template<> constexpr const char *my_typename<uint16_t	> = "uint16_t"	;
template<> constexpr const char *my_typename<uint32_t	> = "uint32_t"	;
template<> constexpr const char *my_typename<uint64_t	> = "uint64_t"	;

#endif

