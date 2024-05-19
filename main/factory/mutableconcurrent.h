#include "mutablebase.h"
#include "arenaallocator.h"

#include "concurrentflushlist.h"

namespace DBAdapterFactory{

	template<DualListEraseType ET, class MyMemList, class MyPairBuffer>
	struct MutableConcurrent{
		using MemList		= MyMemList;

		using MutableBase_	= MutableBase<ET, MemList, MyPairBuffer, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		MutableConcurrent(UStringPathData &&path_data, typename MemList::Allocator &allocator1, typename MemList::Allocator &allocator2, MyPairBuffer &pairBuffer) :
					memList1_{ allocator1 },
					memList2_{ allocator2 },
					base_{
						std::forward<UStringPathData>(path_data),
						memList1_,
						memList2_,
						pairBuffer
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

