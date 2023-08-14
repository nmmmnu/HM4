#include "mutablebase.h"
#include "memlist.h"
#include "arenaallocator.h"

namespace DBAdapterFactory{

	template<class AllocatorX>
	struct Mutable{
		using Allocator		= AllocatorX;

		using MemList		= MyMemList<Allocator>;

		using MutableBase_	= MutableBase<MemList, hm4::FlushList>;

		using MyDBAdapter	= typename MutableBase_::MyDBAdapter;

		template<typename UStringPathData>
		Mutable(UStringPathData &&path_data, Allocator &allocator) :
					memList_{ allocator },
					base_{
						std::forward<UStringPathData>(path_data),
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

