#include "mutablebase.h"
#include "skiplist.h"
#include "concurrentflushlist.h"

namespace DBAdapterFactory{

	struct MutableConcurrent{
		using MemList		= hm4::SkipList;

		using MutableBase_	= MutableBase<MemList, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		MutableConcurrent(UStringPathData &&path_data, size_t const memListSize, MyAllocator::PMAllocator &allocator1, MyAllocator::PMAllocator &allocator2) :
					memList1_{ allocator1 },
					memList2_{ allocator2 },
					base_{
						std::forward<UStringPathData>(path_data),
						memListSize,
						memList1_,
						memList2_
					}{}

		auto &operator()(){
			return base_();
		}

	private:
		MemList	memList1_	;
		MemList	memList2_	;

		MutableBase_	base_	;
	};

}

