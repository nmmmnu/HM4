#include <iostream>

size_t calcAlign__(size_t const value, size_t const align){
//	return align - value % align;
	return align * ((value + align - 1) / align);
}

int main(){
	size_t const align_size = 1;

	for(size_t i = 0; i < 32; ++i){
		size_t const index = i;

		size_t const align = calcAlign__(i, align_size);

		std::cout << index << ' ' << align << '\n';
	}
}

