#include "db_builder_base.cc"

#include "unsortedlist.h"
#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "mmaparenaallocator.h"

using MMapAllocator = MyAllocator::MMapAllocator;
using Allocator     = MyAllocator::MMapArenaAllocator;

constexpr size_t MIN_ARENA_SIZE = 128;



template<class Allocator>
struct ListFactory{
	using MemList		= hm4::UnsortedList<Allocator>;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	template<typename UString>
	ListFactory(UString &&path, typename MemList::Allocator &allocator) :
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
	using MyListFactory = ListFactory<Allocator>;

	if (argc <= 3)
		return printUsage<FileReader, MyListFactory::MemList, Allocator>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	MMapAllocator mmap_allocator{ max_memlist_arena * MB };

	Allocator allocator{ mmap_allocator.size(), mmap_allocator };

	return process<FileReader>(
			MyListFactory{ path, allocator },
			filename,
			blob
	);
}


