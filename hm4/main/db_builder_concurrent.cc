#include "db_builder_base.cc"

#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "concurrentflushlist.h"

#include "filereader.h"

using MyReader = FileReader;

#include "arenaallocator.h"
#include "mmapbuffer.h"

using ArenaBuffer	= MyBuffer::ByteMMapBuffer;
using Allocator		= MyAllocator::ArenaAllocator;

using MyPairBuffer	= MyBuffer::MMapBuffer<hm4::PairBuffer>;

#if 1
	#include "avllist.h"
	using MyMemList = hm4::AVLList<Allocator>;
#else
	#include "skiplist.h"
	using MyMemList = hm4::SkipList<Allocator>;
#endif

constexpr size_t	MIN_ARENA_SIZE		= 264;

static_assert(MIN_ARENA_SIZE * 1024 * 1024 > hm4::Pair::maxBytes());



template<class MyMemList, class MyPairBuffer>
struct ListFactory{
	using MemList		= MyMemList;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::ConcurrentFlushList<
					hm4::multi::DualListEraseType::TOMBSTONE,
					MemList,
					MyPairBuffer,
					Predicate,
					Flush
				>;

	template<typename UString>
	ListFactory(UString &path, typename MemList::Allocator &allocator1, typename MemList::Allocator &allocator2, MyPairBuffer &pairBuffer) :
				memlist1{ allocator1 },
				memlist2{ allocator2 },
				mylist{
					memlist1,
					memlist2,
					pairBuffer,
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
	using MyListFactory = ListFactory<MyMemList,MyPairBuffer>;

	if (argc <= 3)
		return printUsage<FileReader, MyListFactory::MemList, Allocator>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	ArenaBuffer buffer1{ max_memlist_arena * MB };
	ArenaBuffer buffer2{ max_memlist_arena * MB };

	Allocator	allocator1{ buffer1 };
	Allocator	allocator2{ buffer2 };

	MyPairBuffer	pairBuffer;

	return process<FileReader>(
			MyListFactory{ path, allocator1, allocator2, pairBuffer },
			filename,
			blob
	);
}





