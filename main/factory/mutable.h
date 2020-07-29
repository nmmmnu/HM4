#include "basicmutable.h"

namespace DBAdapterFactory{

	struct Mutable{
		using MemList		= hm4::SkipList;

		using BasicMutable_	= BasicMutable<MemList>;

		using MyDBAdapter	= BasicMutable_::DBAdapter;

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

		BasicMutable_	base_		;
	};

}

