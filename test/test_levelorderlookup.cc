#include "levelorderlookup.h"

#include <cstdint>
#include <cstdio>

int main(){
	using size_type = unsigned short int;

	constexpr auto ll = LevelOrderLookupFactory<uint8_t, 3>::build();

	for(size_type i = 0; i < ll.size(); ++i)
		printf("%4u %4u\n", i, ll[i] );
}

