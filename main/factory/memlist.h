#if 1
	#include "avllist.h"
	template<class Allocator>
	using MyMemList = hm4::AVLList<Allocator>;
#else
	#include "skiplist.h"
	template<class Allocator>
	using MyMemList = hm4::SkipList<Allocator>;
#endif

