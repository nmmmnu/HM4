#ifndef MY_ALIGH_H_
#define MY_ALIGH_H_

#include <cstdint>
#include <ostream>

template<size_t ALIGN = sizeof(uint64_t)>
class MyAlign{
private:
	constexpr static char FILL = 0x00;

public:
	constexpr MyAlign(){
		for(size_t i = 0; i < ALIGN; ++i)
			buffer_[i] = FILL;
	}

public:
	constexpr size_t align() const{
		return ALIGN;
	}

	constexpr size_t calc(size_t const bytes) const{
		return ALIGN * ((bytes + ALIGN - 1) / ALIGN);
	}

	constexpr size_t padding(size_t const bytes) const{
		return calc(bytes) - bytes;
	}

public:
	size_t fwriteGap(std::ostream &os, size_t const size) const{
		size_t const gap = padding(size);

		os.write(buffer_, (std::streamsize) gap);

		return gap;
	}

private:
	char	buffer_[ALIGN] = {};

};

#endif

