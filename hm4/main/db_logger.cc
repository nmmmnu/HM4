#include "db_builder_base.cc"

#include "blackholelist.h"
#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

#include "filereader.h"

using MyReader = FileReader;

struct MyListFactory{
	using MemList	= hm4::BlackHoleList;
	using BinLogger	= hm4::binlogger::DiskFileBinLogger;
	using MyList	= hm4::BinLogList<MemList, BinLogger, /* unlink */ false>;

	MyListFactory(std::string_view const binLogFilename, hm4::Pair::WriteOptions const writeOptions) :
				mylist{
					memlist,
					BinLogger{ binLogFilename, writeOptions }
				}{}

	MyList &operator()(){
		return mylist;
	}

private:
	MemList		memlist;
	MyList		mylist;
};



int main(int argc, char **argv){
	if (argc <= 2)
		return printUsage<FileReader>(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 4 && argv[3][0] == 'b';

	return process<FileReader>(
			MyListFactory{ path, hm4::Pair::WriteOptions::ALIGNED },
			filename,
			blob
	);
}



