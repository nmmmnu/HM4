#define REPLAYLIST_AVL
//#define REPLAYLIST_SKIP
//#define REPLAYLIST_UNSORTED

#include "db_builder_base.cc"

#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "disk/disklist.h"

#include "arenaallocator.h"
#include "mmapbuffer.h"

#include "binlogreplay.h"

using Allocator		= MyAllocator::ArenaAllocator;

constexpr size_t	MIN_LIST_ARENA_SIZE	= 264;
constexpr size_t	MIN_HASH_ARENA_SIZE	= 8;

static_assert(MIN_LIST_ARENA_SIZE * MB >= hm4::Pair::maxBytes());



#if defined REPLAYLIST_AVL
	#include "avllist.h"

	using MyReplayList = hm4::AVLList<Allocator>;
#elif defined MEMLIST_SKIP
	#include "skiplist.h"

	using MyReplayList = hm4::SkipList<Allocator>;
#elif defined MEMLIST_UNSORTED
	#include "unsortedlist.h"

	using MyReplayList = hm4::UnsortedList<Allocator>;
#else
	#error "No net::replaylist selected!"
#endif



// same as in db_net

template<class MyMemList>
struct BinLogReplay{
	using MemList		= MyMemList;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	template<typename UString>
	BinLogReplay(UString &&path, typename MemList::Allocator &allocator,
						hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite,
						MyBuffer::ByteBufferView bufferPair, MyBuffer::ByteBufferView bufferHash) :
				memlist{ allocator },
				mylist{
					memlist		,
					buffersWrite	,
					bufferPair	,
					bufferHash	,
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

template<class MyMemList>
using MyListFactory = BinLogReplay<MyMemList>;



namespace{

	int printUsage(const char *cmd){
	       fmt::print(     "db_replay version {version}\n"
			       "\n"
				"Build:\n"
				"\tDate   : {date} {time}\n"
				"\n"
				"Usage:\n"
				"\t{cmd} [file.data] [lsm_path] [memlist arena in MB] [hash arena in MB] [n = import as non aligned] - load file.data, then create / add to lsm_path\n"
				"\t\tPath names must be written with quotes:\n"
				"\t\t\tExample directory/file.'*'.db\n"
				"\t\t\tThe '*', will be replaced with ID's\n"
				"\t\tDo not overcommit memlist arena!\n"
				"\n",
				fmt::arg("version",	hm4::version::str	),
				fmt::arg("date",	__DATE__		),
				fmt::arg("time",	__TIME__		),
				fmt::arg("cmd",		cmd			)
	       );

	       return 10;
	}

}



#include "disk/filebuilder.misc.h"
// defines g_fbwb;



int main(int argc, char **argv){
	using MyBuffer::MMapMemoryResource;

	if (argc <= 4)
		return printUsage(argv[0]);

	const char *inputFile		= argv[1];
	const char *path		= argv[2];

	size_t const arenaListSize	= std::max(from_string<size_t>(argv[3]), MIN_LIST_ARENA_SIZE);

	size_t const arenaHashSize_	= from_string<size_t>(argv[4]);
	size_t const arenaHashSize  	= arenaHashSize_ < MIN_HASH_ARENA_SIZE ? 0 : arenaHashSize_;

	bool const non_aligned		= argc >= 6 && argv[5][0] == 'n';

	MMapMemoryResource	buffer{ arenaListSize * MB };

	Allocator		allocator{ buffer };

	MMapMemoryResource	bufferPair{ hm4::Pair::maxBytes() };
	MMapMemoryResource	bufferHash{ arenaHashSize * MB };

	auto buffersWrite = g_fbwb();

	MyListFactory<MyReplayList> factory{ path, allocator, buffersWrite, bufferPair, bufferHash };

	auto &mylist = factory();

	using hm4::disk::DiskList;

	DiskList input;

	auto const result = input.openForRepair(inputFile, ! non_aligned);

	if (! result){
		fmt::print("Can not open input table.\n", inputFile);
		return 2;
	}

	hm4::binlogFileReplay(mylist, input);
}


