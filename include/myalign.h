#ifndef MY_ALIGH_H_
#define MY_ALIGH_H_

#include <cstdint>
#include <ostream>

class MyAlign{
public:
	constexpr MyAlign() = default;

	constexpr MyAlign(uint16_t align) : align_(align){}

public:
	constexpr uint16_t align() const{
		return align_;
	}

	constexpr size_t calc(size_t const bytes) const{
		return calc__(bytes, align_);
	}

	constexpr size_t padding(size_t const bytes) const{
		return calc(bytes) - bytes;
	}

public:
	size_t fwriteGap(std::ostream &os, size_t const size) const{
		size_t const gap = padding(size);

		// this seems to be safer way
		for(size_t i = 0; i < gap; ++i)
			os.put(0);

		return gap;
	}

private:
	constexpr static size_t calc__(size_t const bytes, uint16_t const align){
		return align * ((bytes + align - 1) / align);
	}

private:
	uint16_t align_ = sizeof(uint64_t);
};

#endif

