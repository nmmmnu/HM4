#include <algorithm>

#include "version.h"

#include "unsortedlist.h"
#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfilepredicate.h"
#include "flusher/diskfileflush.h"
#include "flushlist.h"

#include "filereader.h"
#include "stringtokenizer.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"

#include "logger.h"

#include "base64.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using MyArenaAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;
using MySTDAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

constexpr size_t	MB			= 1024 * 1024;

constexpr size_t	MIN_ARENA_SIZE		= 128;

constexpr size_t	PROCESS_STEP		= 10'000;

constexpr char		DELIMITER		= '\t';

using MyReader		= FileReader;

using MyReaderBuffer	= std::array<char, 16 * 1024>;

MyReaderBuffer	reader_buffer, base64_buffer;



static int printUsage(const char *cmd);



template <class LIST, class READER, bool B>
int listLoad(LIST &list, READER &reader, size_t const process_step, std::bool_constant<B> tag);



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

	const char *filename	= argv[1];
	const char *path	= argv[2];
	size_t const max_memlist_arena = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE);

	bool const blob = argc >= 5 && argv[4][0] == 'b';

	MyArenaAllocator allocator{ max_memlist_arena * MB };

	MyListFactory factory{ path, allocator };

	auto &mylist = factory();

	MyReader reader{ filename, reader_buffer.data(), reader_buffer.size() };

	if (blob == false)
		return listLoad(mylist, reader, PROCESS_STEP, std::false_type{});
	else
		return listLoad(mylist, reader, PROCESS_STEP, std::true_type{});
}



template <class LIST, bool B>
auto listInsert(LIST &list, std::string_view const key, std::string_view const val, std::bool_constant<B>){
	if constexpr(B)
		return list.insert(key, base64_decode(val, base64_buffer.data()));
	else
		return list.insert(key, val);
}

template <class LIST, class READER, bool B>
int listLoad(LIST &list, READER &reader, size_t const process_step, std::bool_constant<B> tag){
	size_t i = 0;

	while(reader){
		// reader will keep line alive...
		std::string_view const line = reader.getLine();

		StringTokenizer const st{ line, DELIMITER };

		auto tok = getForwardTokenizer(st);

		std::string_view const key = tok();
		std::string_view const val = tok();

		if (! key.empty()){
			if ( listInsert(list, key, val, tag) == std::end(list) )
				log__("Error insert", key);
		}

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
	fmt::print(	"db_builder version {0}\n"
			"\n"
			"Usage:\n"
			"\t{1} [file.txt] [lsm_path] [memlist arena in MB] [b = import as base64 blobs] - load file.txt, then create / add to lsm_path\n"
			"\t\tPath names must be written with quotes:\n"
			"\t\tExample directory/file.'*'.db\n"
			"\t\tThe '*', will be replaced with ID's\n"
			"\n"
			"\tReader: {2}\n"
			"\tBuffer: {3}\n"
			"\n",
			hm4::version::str,
			cmd,
			MyReader::name(),
			reader_buffer.size()
	);

	return 10;
}


