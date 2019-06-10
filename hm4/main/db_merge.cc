#include "disk/filebuilder.h"

#include <unistd.h>	// access

#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< '\t'		<< cmd	<< " - output.db [file1.db] [file2.db] [fileN.db] - merge files, keep   tombstones"	<< '\n'
		<< '\t'		<< cmd	<< " t output.db [file1.db] [file2.db] [fileN.db] - merge files, remove tombstones"	<< '\n'

		<< "\t\tPath names must be written like this:"			<< '\n'
		<< "\t\tExample 'directory/file.db'"				<< '\n'
		<< "\t\tDo not forget you usually need two output files"	<< '\n'

		<< '\n';

	return 10;
}

inline bool fileExists(const StringRef& name) {
	return access(name.data(), F_OK) != -1;
}

template <class FACTORY>
int mergeFromFactory(const FACTORY &f, const char *output_file, bool const keepTombstones){
	hm4::disk::FileBuilder::build(output_file, f.begin(), f.end(), keepTombstones, /* aligned */ true);

	return 0;
}

#include "listloader/arglistloader.h"

#include "multi/duallist.h"
#include "multi/collectionlist.h"

using hm4::disk::DiskList;

struct MergeListFactory_1{
	MergeListFactory_1(const char *filename, MMAPFile::Advice const advice, DiskList::OpenMode const mode){
		table_.open(filename, advice, mode);
	}

	auto begin() const{
		return std::begin(table_);
	}

	auto end() const{
		return std::end(table_);
	}

private:
	DiskList table_;
};

struct MergeListFactory_2{
	MergeListFactory_2(const char *filename1, const char *filename2, const MMAPFile::Advice advice, DiskList::OpenMode const mode){
		table1_.open(filename1, advice, mode);
		table2_.open(filename2, advice, mode);
	}

	auto begin() const{
		return std::begin(table_);
	}

	auto end() const{
		return std::end(table_);
	}

private:
	using MyDualList = hm4::multi::DualList<const DiskList, const DiskList>;

	DiskList	table1_;
	DiskList	table2_;
	MyDualList	table_{ table2_, table1_ };
};

struct MergeListFactory_N{
	MergeListFactory_N(int const table_count, const char **path, const MMAPFile::Advice advice, DiskList::OpenMode const mode) :
					loader_(table_count, path, advice, mode){}

	auto begin() const{
		return std::begin(loader_.getList());
	}

	auto end() const{
		return std::end(loader_.getList());
	}

private:
	hm4::listloader::ArgListLoader	loader_;
};



constexpr auto	DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto	DEFAULT_MODE	= DiskList::OpenMode::MINIMAL;

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	const char *output	= argv[2];

	if (fileExists(output)){
		printf("File %s exists. Please remove it and try again.\n", output);
		return 2;
	}

	bool const keepTombstones = argv[1][0] == 't' ? false : true;

	int const table_count	= argc - 3;
	const char **path	= (const char **) &argv[3];


	switch(table_count){
	case 1:
		{
			std::cout
				<< "Merging (cleanup) single table..."	<< '\n'
				<< '\t' << path[0]			<< '\n'
			;

			MergeListFactory_1 factory{ path[0], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);
		}

	case 2:
		{
			std::cout
				<< "Merging two tables..."		<< '\n'
				<< '\t' << path[0]			<< '\n'
				<< '\t' << path[1]			<< '\n'
			;

			MergeListFactory_2 factory{ path[0], path[1], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);

		}

	default:
		{
			std::cout
				<< "Merging multiple tables..."		<< '\n'
			;

			MergeListFactory_N factory{ table_count, path, DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);
		}
	}
}

