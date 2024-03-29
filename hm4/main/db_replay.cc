#include "db_builder_base.cc"

#include "unsortedlist.h"
#include "idgenerator.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "disk/disklist.h"

#include "arenaallocator.h"
#include "mmapallocator.h"
#include "allocatedbuffer.h"

using ArenaBuffer	= MyBuffer::AllocatedByteBufferOwned<MyAllocator::MMapAllocator>;
using Allocator		= MyAllocator::ArenaAllocator;

constexpr size_t	MIN_ARENA_SIZE		= 128;



struct MyListFactory{
	using MemList		= hm4::UnsortedList<Allocator>;
	using Predicate		= hm4::flusher::DiskFileAllocatorPredicate;
	using IDGenerator	= idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate, Flush>;

	template<typename UString>
	MyListFactory(UString &&path, MemList::Allocator &allocator, hm4::PairBuffer &pairBuffer) :
				memlist{ allocator },
				mylist{
					memlist,
					pairBuffer,
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



template <class List, class InputList>
int listReplay(List &list, InputList const &inputList, size_t const process_step){
	size_t i = 0;

	for(auto const &pair : inputList){
		if (!pair.isKeyEmpty())
			insert(list, pair);

		++i;

		if (i % process_step == 0)
			printStats<false>(list, i);
	}

	return 0;
}



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
	if (argc <= 3)
		return printUsage(argv[0]);

	const char *inputFile	= argv[1];
	const char *path	= argv[2];

	bool const non_aligned = argc >= 5 && argv[4][0] == 'n';

	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	ArenaBuffer	buffer{ max_memlist_arena * MB };

	Allocator	allocator{ buffer };

	auto pairBuffer = std::make_unique<hm4::PairBuffer>();

	MyListFactory factory{ path, allocator, *pairBuffer };

	auto &mylist = factory();

	using hm4::disk::DiskList;

	DiskList input;

	auto const result = input.openDataOnly_with_bool(inputFile, ! non_aligned);

	if (! result){
		fmt::print("Can not open input table.\n", inputFile);
		return 2;
	}

	return listReplay(mylist, input, PROCESS_STEP);
}


