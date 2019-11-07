#ifndef MY_ALIGH_H_
#define MY_ALIGH_H_

#include <cstddef>
#include <ostream>

namespace my_align{
	/*
	constexpr size_t calc(size_t const bytes, size_t const align){
		return align * ((bytes + align - 1) / align);
	}
	*/

	// http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator
	constexpr size_t align(size_t n, size_t align){
		return (n + align - 1) & ~(align - 1);
	}

	constexpr size_t calc(size_t n, size_t align){
		return (n + align - 1) & ~(align - 1);
	}

	inline size_t fwriteGap(std::ostream &os, size_t const bytes, size_t const align){
		size_t const gap = calc(bytes, align) - bytes;

		for(size_t i = 0; i < gap; ++i)
			os.put(0);

		return gap;
	}

} // namespace my_align

#endif

