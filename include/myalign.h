#ifndef MY_ALIGH_H_
#define MY_ALIGH_H_

#include <cstdint>
#include <ostream>

template<uint16_t ALIGN = sizeof(uint64_t)>
class MyAlign{
private:
	constexpr static char FILL = 0x00;

public:
	constexpr MyAlign(){
		for(uint16_t i = 0; i < ALIGN; ++i)
			buffer_[i] = FILL;
	}

public:
	constexpr uint16_t align() const{
		return ALIGN;
	}

	constexpr size_t calc(size_t const bytes) const{
		return calc__(bytes, ALIGN);
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
	constexpr static size_t calc__(size_t const bytes, uint16_t const align){
		return align * ((bytes + align - 1) / align);
	}

private:
	char	buffer_[ALIGN] = {};

};

#endif

