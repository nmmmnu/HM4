#include "db_builder_base.cc"

#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "concurrentflushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "arenaallocator.h"
#include "mmapbuffer.h"

using Allocator		= MyAllocator::ArenaAllocator;

#if 1
	#include "avllist.h"
	using MyMemList = hm4::AVLList<Allocator>;
#else
	#include "skiplist.h"
	using MyMemList = hm4::SkipList<Allocator>;
#endif

constexpr size_t	MIN_LIST_ARENA_SIZE	= 264;
constexpr size_t	MIN_HASH_ARENA_SIZE	= 8;

static_assert(MIN_LIST_ARENA_SIZE * MB >= hm4::Pair::maxBytes());



template<class MyMemList>
struct ListFactory{
	using MemList		= MyMemList;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::ConcurrentFlushList<
					hm4::multi::DualListEraseType::TOMBSTONE,
					MemList,
					Predicate,
					Flush
				>;

	template<typename UString>
	ListFactory(UString &path, typename MemList::Allocator &allocator1, typename MemList::Allocator &allocator2,
						hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite,
						MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash) :
				memlist1{ allocator1 },
				memlist2{ allocator2 },
				mylist{
					memlist1	,
					memlist2	,
					buffersWrite	,
					bufferPair	,
					bufferHash	,
					Predicate{}	,
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



#include "disk/filebuilder.misc.h"
// defines g_fbwb;



int main(int argc, char **argv){
	using MyBuffer::MMapMemoryResource;

	using MyListFactory = ListFactory<MyMemList>;

	if (argc <= 4)
		return printUsage<FileReader, MyListFactory::MemList, Allocator>(argv[0]);

	const char *filename		= argv[1];
	const char *path		= argv[2];

	size_t const arenaListSize	= std::max(from_string<size_t>(argv[3]), MIN_LIST_ARENA_SIZE);

	size_t const arenaHashSize_	= from_string<size_t>(argv[4]);
	size_t const arenaHashSize  	= arenaHashSize_ < MIN_HASH_ARENA_SIZE ? 0 : arenaHashSize_;

	bool const blob 		= argc >= 6 && argv[5][0] == 'b';

	MMapMemoryResource	buffer1{ arenaListSize * MB };
	MMapMemoryResource	buffer2{ arenaListSize * MB };

	Allocator		allocator1{ buffer1 };
	Allocator		allocator2{ buffer2 };

	MMapMemoryResource	bufferPair{ hm4::Pair::maxBytes() };
	MMapMemoryResource	bufferHash{ arenaHashSize * MB };

	auto buffersWrite = g_fbwb();

	return process<FileReader>(
			MyListFactory{ path, allocator1, allocator2, buffersWrite, bufferPair, bufferHash },
			filename,
			blob
	);
}


