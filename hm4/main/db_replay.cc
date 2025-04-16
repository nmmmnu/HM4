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

constexpr size_t	MIN_ARENA_SIZE		= 264;

static_assert(MIN_ARENA_SIZE * 1024 * 1024 > hm4::Pair::maxBytes());



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
	BinLogReplay(UString &&path, typename MemList::Allocator &allocator, MyBuffer::ByteBufferView bufferPair) :
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
				"\t{cmd} [file.data] [lsm_path] [memlist arena in MB] [n = import as non aligned] - load file.data, then create / add to lsm_path\n"
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



int main(int argc, char **argv){
	using MyBuffer::MMapBufferResource;

	if (argc <= 3)
		return printUsage(argv[0]);

	const char *inputFile		= argv[1];
	const char *path		= argv[2];

	bool const non_aligned		= argc >= 5 && argv[4][0] == 'n';

	size_t const memlist_arena	= std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	MMapBufferResource	buffer{ memlist_arena * MB };

	Allocator		allocator{ buffer };

	MMapBufferResource	bufferPair{ hm4::Pair::maxBytes() };

	MyListFactory<MyReplayList> factory{ path, allocator, bufferPair };

	auto &mylist = factory();

	using hm4::disk::DiskList;

	DiskList input;

	auto const result = input.openDataOnly_with_bool(inputFile, ! non_aligned);

	if (! result){
		fmt::print("Can not open input table.\n", inputFile);
		return 2;
	}

	hm4::binlogFileReplay(mylist, input);
}


