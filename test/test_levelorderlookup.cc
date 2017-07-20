#include "levelorderlookup.h"

#include <cstdio>

int main(){
	using size_type = unsigned short int;

	constexpr LevelOrderLookup<3> ll;

	for(size_type i = 0; i < ll.size(); ++i)
		printf("%4u %4u\n", i, ll[i] );
}

