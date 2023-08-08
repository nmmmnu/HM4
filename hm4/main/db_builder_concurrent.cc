#include "db_builder_base.cc"

#include "unrolledskiplist.h"
#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "concurrentflushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "arenaallocator.h"

// Yay, non virtual :)
using MyArenaAllocator = MyAllocator::ArenaAllocator;

constexpr size_t MIN_ARENA_SIZE = 128;



struct MyListFactory{
	using MemList		= hm4::UnrolledSkipList<MyArenaAllocator>;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::ConcurrentFlushList<MemList,Predicate,Flush>;

	template<typename UString>
	MyListFactory(UString &path, MyArenaAllocator &allocator1, MyArenaAllocator &allocator2) :
				memlist1{ allocator1 },
				memlist2{ allocator2 },
				mylist{
					memlist1,
					memlist2,
					Predicate{},
					Flush{ IDGenerator{}, std::forward<UString>(path) }
				}{}

	MyList &operator()(){
		return mylist;
	}

private:
	MemList	memlist1;
	MemList	memlist2;
	MyList	mylist;
};



int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage<FileReader>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	MyArenaAllocator allocator1{ max_memlist_arena * MB };
	MyArenaAllocator allocator2{ max_memlist_arena * MB };

	return process<FileReader>(
			MyListFactory{ path, allocator1, allocator2 },
			filename,
			blob
	);
}





