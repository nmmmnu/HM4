#include <iostream>
#include <iomanip>

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


using Allocator_s = MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;
using Allocator_a = MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;

using Allocator = Allocator_a;

constexpr size_t	CHUNK_SIZE		= 1ULL * 2 * 1024 * 1024 * 1024;

Allocator allocator{ CHUNK_SIZE };

constexpr size_t	MEMLIST_SIZE		= CHUNK_SIZE;
constexpr size_t	PROCESS_STEP		= 1000 * 10;

constexpr size_t	READER_BUFFER_SIZE	= 16 * 1024;

constexpr char		DELIMITER		= '\t';

using MyReader		= FileReader<READER_BUFFER_SIZE>;

static int printUsage(const char *cmd);


template <class LIST, class READER>
int listLoad(LIST &list, READER &reader, size_t process_step);


struct MyListFactory{
	using MemList		= hm4::UnsortedList;
	using Predicate		= hm4::flusher::DiskFilePredicate;
	using IDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flush		= hm4::flusher::DiskFileFlush<IDGenerator>;
	using MyList		= hm4::FlushList<MemList,Predicate,Flush>;

	MyListFactory(const char *path, size_t const memlist_size) : mylist(
			memlist,
			Predicate{ memlist_size },
			Flush{ IDGenerator{}, path }
		){}

	MyList &operator()(){
		return mylist;
	}

private:
	MemList	memlist{ allocator };
	MyList	mylist;
};


int main(int argc, char **argv){
	if (argc <= 2)
		return printUsage(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	MyListFactory factory{ path, MEMLIST_SIZE };

	auto &mylist = factory();

	MyReader reader{ filename };

	return listLoad(mylist, reader, PROCESS_STEP);
}



template <class LIST, class READER>
int listLoad(LIST &list, READER &reader, size_t const process_step){
	size_t i = 0;

	while(reader){
		// reader will keep line alive...
		std::string_view const line = reader.getLine();

		StringTokenizer const st{ line, DELIMITER };

		auto tok = getForwardTokenizer(st);

		std::string_view const key = tok();
		std::string_view const val = tok();

	//	std::cout << key << ':' << val << '\n';

		if (! key.empty())
			list.insert(key, val);

		++i;

		if (i % process_step == 0){
			auto used = list.getAllocator().getUsedMemory();

			if (used != std::numeric_limits<decltype(used)>::max()){
				std::clog
					<< "Processed "	<< std::setw(10) << i			<< " records."	<< ' '
					<< "In memory "	<< std::setw(10) << list.size()		<< " records,"	<< ' '
							<< std::setw(10) << list.bytes()	<< " bytes."	<< ' '
					<< "Allocator "	<< std::setw(10) << used		<< " bytes."	<< '\n'
				;
			}else{
				std::clog
					<< "Processed "	<< std::setw(10) << i			<< " records."	<< ' '
					<< "In memory "	<< std::setw(10) << list.size()		<< " records,"	<< ' '
							<< std::setw(10) << list.bytes()	<< " bytes."	<< '\n'
				;
			}
		}
	}

	return 0;
}



static int printUsage(const char *cmd){
	std::cout
		<< "db_builder version " << hm4::version::str 								<< '\n'
		<< '\n'
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " [file.txt] [lsm_path] - load file.txt, then create / add to lsm_path"	<< '\n'

		<< "\t\tPath names must be written with quotes:"	<< '\n'
		<< "\t\tExample directory/file.'*'.db"			<< '\n'
		<< "\t\tThe '*', will be replaced with ID's"		<< '\n'
		<< '\n'
		<< "Settings:"						<< '\n'
		<< "\tReader: "	<< MyReader::name()			<< '\n'
		<< "\tBuffer: "	<< READER_BUFFER_SIZE			<< '\n'

		<< '\n';

	return 10;
}

