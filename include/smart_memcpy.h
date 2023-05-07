#ifndef SMART_MEMCPY_H_
#define SMART_MEMCPY_H_

#include <cassert>
#include <cstring>

inline void smart_memcpy(void *dest_v, const void *src, size_t const src_size, size_t const dest_size){
	assert(src_size <= dest_size);

	char *dest = reinterpret_cast<char *>(dest_v);

	memcpy(dest,		src,	src_size		);
	memset(dest + src_size, '\0',	dest_size - src_size	);

	// const char *mask = "%s %10zu %10zu\n";
	// printf(mask, "memcpy",	size_t{ 0 },	src_size		);
	// printf(mask, "memset",	src_size,	dest_size - src_size	);
}

#endif

