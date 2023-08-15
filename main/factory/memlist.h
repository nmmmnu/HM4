#if 1
	#include "avllist.h"

	namespace DBAdapterFactory{
		template<class Allocator>
		using MyMemList = hm4::AVLList<Allocator>;
	}
#else
	#include "skiplist.h"

	namespace DBAdapterFactory{
		template<class Allocator>
		using MyMemList = hm4::SkipList<Allocator>;
	}
#endif

