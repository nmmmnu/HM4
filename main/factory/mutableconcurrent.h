#include "mutablebase.h"
#include "arenaallocator.h"

#include "concurrentflushlist.h"

namespace DBAdapterFactory{

	template<DualListEraseType ET, class MyMemList>
	struct MutableConcurrent{
		using MemList		= MyMemList;

		using MutableBase_	= MutableBase<ET, MemList, hm4::ConcurrentFlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		MutableConcurrent(
				UStringPathData			&&path_data	,
				DiskList::VMAllocator		&slabAllocator	,
				typename MemList::Allocator	&allocator1	,
				typename MemList::Allocator	&allocator2	,
				MyBuffer::ByteBufferView	pairBuffer	,
				MyBuffer::ByteBufferView	bufferHash
			) :
					memList1_{ allocator1 },
					memList2_{ allocator2 },
					base_{
						std::forward<UStringPathData>(path_data),
						slabAllocator	,
						memList1_	,
						memList2_	,
						pairBuffer	,
						bufferHash
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

