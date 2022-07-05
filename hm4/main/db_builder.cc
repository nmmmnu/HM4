#include "db_builder_base.cc"

#include "unsortedlist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "arenaallocator.h"

// Yay, non virtual :)
using MyArenaAllocator = MyAllocator::ArenaAllocator;

constexpr size_t MIN_ARENA_SIZE = 128;



struct MyListFactory{
	using MemList		= hm4::UnsortedList<MyArenaAllocator>;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	template<typename UString>
	MyListFactory(UString &&path, MyArenaAllocator &allocator) :
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


int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage<FileReader>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	MyArenaAllocator allocator{ max_memlist_arena * MB };

	return process<FileReader>(
			MyListFactory{ path, allocator },
			filename,
			blob
	);
}


