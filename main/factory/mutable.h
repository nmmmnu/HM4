#include "mutablebase.h"
#include "arenaallocator.h"

namespace DBAdapterFactory{

	template<DualListEraseType ET, class MyMemList>
	struct Mutable{
		using MemList		= MyMemList;

		using MutableBase_	= MutableBase<ET, MemList, hm4::FlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		Mutable(UStringPathData &&path_data, typename MemList::Allocator &allocator, MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash) :
					memList_{ allocator },
					base_{
						std::forward<UStringPathData>(path_data),
						memList_,
						bufferPair,
						bufferHash
					}{}

		auto &operator()(){
			return base_();
		}

	private:
		MemList		memList_	;

		MutableBase_	base_		;
	};

}

