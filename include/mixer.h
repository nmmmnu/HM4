#ifndef MY_MIXER_H_
#define MY_MIXER_H_

#include <cstdint>

uint64_t mixer(uint64_t x){
	x ^= x >> 33;
	x *= 0xff51afd7'ed558ccd;
	x ^= x >> 33;
	x *= 0xc4ceb9fe'1a85ec53;
	x ^= x >> 33;
	return x;
}

#endif

