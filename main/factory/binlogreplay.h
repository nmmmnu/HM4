#ifndef BIN_LOG_REPLAY_H_
#define BIN_LOG_REPLAY_H_

#include "unsortedlist.h"
#include "arenaallocator.h"

namespace DBAdapterFactory{

	template<class Allocator>
	struct BinLogReplay{
		using MemList		= hm4::UnsortedList<Allocator>;
		using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
		using IDGenerator	= idgenerator::IDGeneratorDate;
		using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
		using MyList		= hm4::FlushList<MemList,Predicate, Flush>;

		template<typename UString>
		BinLogReplay(UString &&path, Allocator &allocator) :
					memlist{ allocator },
					mylist{
						memlist,
						Predicate{},
						Flush{ IDGenerator{}, std::forward<UString>(path) }
					}{}

		MyList &operator()(){
			return mylist;
		}

	private:
		MemList	memlist;
		MyList	mylist;
	};

}

#endif

