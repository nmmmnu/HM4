#include <iostream>
#include <iomanip>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " [file.txt] [lsm_path] - load file.txt, then create / add to lsm_path"		<< '\n'

		<< "\t\tPath names must be written without extention"		<< '\n'
		<< "\t\tExample 'directory/file.*.db'"				<< '\n'

		<< '\n';

	return 10;
}

#include "skiplist.h"
#include "flushlist.h"

#include "idgenerator/idgeneratordate.h"
#include "flusher/diskfileflusher.h"

#include "filereader.h"
#include "stringtokenizer.h"

constexpr size_t	MEMLIST_SIZE	= 10 * 1024 * 1024;
constexpr size_t	PROCESS_STEP	= 1000 * 10;

template <class LIST, class READER>
static int listLoad(LIST &list, READER &reader);

int main(int argc, char **argv){
	if (argc <= 2){
		return printUsage(argv[0]);
	}

	const char *filename	= argv[1];
	const char *path	= argv[2];

	using MemList		= hm4::SkipList;
	using MyIDGenerator	= hm4::idgenerator::IDGeneratorDate;
	using Flusher		= hm4::flusher::DiskFileFlusher<MyIDGenerator>;
	using MyList		= hm4::FlushList<MemList,Flusher>;

	MemList memlist;

	MyList mylist{
		memlist,
		Flusher{
			MyIDGenerator{},
			path,
			".db"
		},
		MEMLIST_SIZE
	};

	FileReader<1024> reader{ filename };

	return listLoad(mylist, reader);
}



template <class LIST, class READER>
static int listLoad(LIST &list, READER &reader){
	size_t i = 0;

	while(reader){
		std::string line = reader.getLine();

		StringTokenizer tok{ line };

		const StringRef &key = tok.getNext();
		const StringRef &val = tok.getNext();

		if (! key.empty())
			list.insert( { key, val } );

		++i;

		if (i % PROCESS_STEP == 0){
			const auto &ll = list.getList();

			std::cout
				<< "Processed "	<< std::setw(10) << i		<< " records." << ' '
				<< "In memory "	<< std::setw(10) << ll.size()	<< " records," << ' '
						<< std::setw(10) << ll.bytes()	<< " bytes." << '\n'
			;
		}
	}

	return 0;
}

