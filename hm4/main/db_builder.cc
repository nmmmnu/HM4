#include <iostream>
#include <iomanip>

#include "version.h"

#include "skiplist.h"
#include "flushlist.h"

#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfileflusher.h"

#include "filereader.h"
#include "stringtokenizer.h"

constexpr size_t	MEMLIST_SIZE	= 1ULL * 2 * 1024 * 1024 * 1024;
constexpr size_t	PROCESS_STEP	= 1000 * 10;

constexpr char		DELIMITER	= '\t';

static int printUsage(const char *cmd);


template <class LIST, class READER>
int listLoad(LIST &list, READER &reader, size_t process_step);


struct MyListFactory{
	using MemList		= hm4::SkipList;
	using MyIDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flusher		= hm4::flusher::DiskFileFlusher<MyIDGenerator>;
	using MyList		= hm4::FlushList<MemList,Flusher>;

	MyListFactory(const char *path, size_t const memlist_size) : mylist(
			memlist,
			Flusher{ MyIDGenerator{}, path },
			memlist_size
		){}

	MyList &operator()(){
		return mylist;
	}

private:
	MemList	memlist;
	MyList	mylist;
};


int main(int argc, char **argv){
	if (argc <= 2)
		return printUsage(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	MyListFactory factory{ path, MEMLIST_SIZE };

	auto &mylist = factory();

	FileReader<1024> reader{ filename };

	return listLoad(mylist, reader, PROCESS_STEP);
}



template <class LIST, class READER>
int listLoad(LIST &list, READER &reader, size_t const process_step){
	size_t i = 0;

	while(reader){
		const std::string &line = reader.getLine();

		StringTokenizer const tok{ line, DELIMITER };

		auto       it  = std::begin(tok);
		auto const end = std::end(tok);

		const StringRef &key = getNextToken(it, end);
		const StringRef &val = getNextToken(it, end);

		// std::cout << key << ':' << val << '\n';

		if (! key.empty())
			list.insert( { key, val } );

		++i;

		if (i % process_step == 0){
			std::cout
				<< "Processed "	<< std::setw(10) << i			<< " records." << ' '
				<< "In memory "	<< std::setw(10) << list.size()		<< " records," << ' '
						<< std::setw(10) << list.bytes()	<< " bytes." << '\n'
			;
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

		<< '\n';

	return 10;
}

