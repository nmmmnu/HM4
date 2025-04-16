#ifndef MY_STATIC_VECTOR_H_
#define MY_STATIC_VECTOR_H_

#include "bufferedvector.h"
#include "staticbuffer.h"

template<typename T, std::size_t Size>
using StaticVector = BufferedVector<T, MyBuffer::StaticBuffer<T, Size> >;

static_assert(std::is_trivially_destructible_v<StaticVector<int, 100> >, "StaticVector must be POD-like type");

#endif

