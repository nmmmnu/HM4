#ifndef SMART_MEMCPY_H_
#define SMART_MEMCPY_H_

#include <cstring>

template<bool MC = true, bool MS = true>
void smart_memcpy(void *dest_v, size_t const dest_size, const void *src, size_t const src_size, char fill = '\0'){
	char *dest = reinterpret_cast<char *>(dest_v);

	if (src_size <= dest_size){

		if constexpr(MC){
			memcpy(dest,		src,	src_size		);
		}

		if constexpr(MS){
			memset(dest + src_size, fill,	dest_size - src_size	);
		}

	}else{

		if constexpr(MC){
			memcpy(dest,		src,	dest_size		);
		}

	}

	#if 0
		const char *mask = "%s %10zu %10zu\n";
		printf(mask, "memcpy",	size_t{ 0 },	src_size		);
		printf(mask, "memset",	src_size,	dest_size - src_size	);
	#endif
}

template<bool MC = true, bool MS = true>
void smart_memcpy(void *dest, size_t const dest_size, std::string_view src, char fill = '\0'){
	return smart_memcpy(dest, dest_size, src.data(), src.size(), fill);
}

#endif

