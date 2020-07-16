#include <algorithm>

#include "version.h"

#include "unsortedlist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"

#include "disk/disklist.h"

#include "logger.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using MyArenaAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;
using MySTDAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

constexpr size_t	MB			= 1024 * 1024;

constexpr size_t	MIN_ARENA_SIZE		= 128;

constexpr size_t	PROCESS_STEP		= 10'000;



static int printUsage(const char *cmd);



template <class LIST, class INPUT_LIST>
int listReplay(LIST &list, INPUT_LIST const &inputList, size_t const process_step);



struct MyListFactory{
	using MemList		= hm4::UnsortedList;
	using Predicate		= hm4::flusher::DiskFilePredicate;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	MyListFactory(std::string_view path, MyAllocator::PMAllocator &allocator) :
				memlist{ allocator },
				mylist{
					memlist,
					Predicate{},
					Flush{ IDGenerator{}, std::string(path) }
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
		return printUsage(argv[0]);

	const char *inputFile	= argv[1];
	const char *path	= argv[2];
	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	bool const non_aligned = argc >= 5 && argv[4][0] == 'n';

	MyArenaAllocator allocator{ max_memlist_arena * MB };

	MyListFactory factory{ path, allocator };

	auto &mylist = factory();

	using hm4::disk::DiskList;

	DiskList input;

	auto const result = input.openDataOnly(inputFile, ! non_aligned);

	if (! result){
		fmt::print("Can not open input table.\n", inputFile);
		return 2;
	}

	return listReplay(mylist, input, PROCESS_STEP);
}



template <class LIST, class INPUT_LIST>
int listReplay(LIST &list, INPUT_LIST const &inputList, size_t const process_step){
	size_t i = 0;

	for(auto const &pair : inputList){
		if (!pair.empty())
			list.insert(pair);
print(pair);
		++i;

		if (i % process_step == 0){
			auto const used = list.getAllocator().getUsedMemory();

			if (used != std::numeric_limits<decltype(used)>::max()){
				fmt::print(stderr,
					"Processed {:15} records. "
					"In memory {:15} records, {:15} bytes. "
					"Allocator {:15} bytes.\n",
					i,
					list.size(), list.bytes(),
					used
				);
			}else{
				fmt::print(stderr,
					"Processed {:15} records. "
					"In memory {:15} records, {:15} bytes.\n",
					i,
					list.size(), list.bytes()
				);
			}
		}
	}

	return 0;
}



static int printUsage(const char *cmd){
	fmt::print(	"db_replay version {0}\n"
			"\n"
			"Usage:\n"
			"\t{1} [file.data] [lsm_path] [memlist arena in MB] [n = import as non aligned] - load file.data, then create / add to lsm_path\n"
			"\t\tPath names must be written with quotes:\n"
			"\t\tExample directory/file.'*'.db\n"
			"\t\tThe '*', will be replaced with ID's\n"
			"\n",
			hm4::version::str,
			cmd
	);

	return 10;
}


