#include "mutablebase.h"
#include "memlist.h"
#include "arenaallocator.h"

#include "concurrentflushlist.h"

namespace DBAdapterFactory{

	template<class AllocatorX>
	struct MutableConcurrent{
		using Allocator		= AllocatorX;

		using MemList		= MyMemList<Allocator>;

		using MutableBase_	= MutableBase<MemList, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		MutableConcurrent(UStringPathData &&path_data, Allocator &allocator1, Allocator &allocator2) :
					memList1_{ allocator1 },
					memList2_{ allocator2 },
					base_{
						std::forward<UStringPathData>(path_data),
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

