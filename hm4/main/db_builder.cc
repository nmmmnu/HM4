#include "db_builder_base.cc"

#include "unsortedlist.h"
#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "arenaallocator.h"
#include "mmapbuffer.h"

using Allocator		= MyAllocator::ArenaAllocator;

constexpr size_t	MIN_ARENA_SIZE		= 264;

static_assert(MIN_ARENA_SIZE * 1024 * 1024 > hm4::Pair::maxBytes());



template<class Allocator>
struct ListFactory{
	using MemList		= hm4::UnsortedList<Allocator>;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	template<typename UString>
	ListFactory(UString &&path, typename MemList::Allocator &allocator, MyBuffer::ByteBufferView bufferPair) :
				memlist{ allocator },
				mylist{
					memlist,
					bufferPair,
					{}, // bufferHash
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
	using MyBuffer::MMapMemoryResource;

	using MyListFactory = ListFactory<Allocator>;

	if (argc <= 3)
		return printUsage<FileReader, MyListFactory::MemList, Allocator>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	size_t const arenaSize	= std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	MMapMemoryResource	buffer{ arenaSize * MB };

	Allocator		allocator{ buffer };

	MMapMemoryResource	bufferPair{ hm4::Pair::maxBytes() };

	return process<FileReader>(
			MyListFactory{ path, allocator, bufferPair },
			filename,
			blob
	);
}


