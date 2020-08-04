#include "mutablebase.h"
#include "skiplist.h"

namespace DBAdapterFactory{

	struct Mutable{
		using MemList		= hm4::SkipList;

		using MutableBase_	= MutableBase<MemList, hm4::FlushList>;

		using MyDBAdapter	= MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		Mutable(UStringPathData &&path_data, size_t const memListSize, MyAllocator::PMAllocator &allocator) :
					memList_{ allocator },
					base_{
						std::forward<UStringPathData>(path_data),
						memListSize,
						memList_
					}{}

		auto &operator()(){
			return base_();
		}

	private:
		MemList		memList_	;

		MutableBase_	base_		;
	};

}

