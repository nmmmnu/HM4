#include "disk/filebuilder.h"

#include <unistd.h>	// access

#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< '\t'		<< cmd	<< " - [file1.db] [file2.db] [fileN.db] - merge files, keep   tombstones"	<< '\n'
		<< '\t'		<< cmd	<< " t [file1.db] [file2.db] [fileN.db] - merge files, remove tombstones"	<< '\n'

		<< "\t\tFile names must be written without extention"		<< '\n'
		<< "\t\tExample 'file.db'"					<< '\n'
		<< "\t\tDo not forget you usually need two output files"	<< '\n'

		<< '\n';

	return 10;
}

inline bool fileExists(const StringRef& name) {
	return access(name.data(), F_OK) != -1;
}

template <class TABLE>
int mergeTable(const TABLE &table, const char *output_file, bool const keepTombstones){
	hm4::disk::FileBuilder df;

	df.build(output_file, table, keepTombstones, /* aligned */ true);

	return 0;
}

template <class FACTORY>
int mergeFromFactory(const FACTORY &f, const char *output_file, bool const keepTombstones){
	const auto &table = f();
	return mergeTable(table, output_file, keepTombstones);
}


#include "tableloader/argtableloader.h"

#include "multi/dualtable.h"
#include "multi/collectiontable.h"

struct MergeTableFactory_1{
	using DiskTable = hm4::disk::DiskTable;
	using MyMergeTable = DiskTable;

	MergeTableFactory_1(const char *filename, int const advice){
		table_.open(filename, advice);
	}

	const MyMergeTable &operator()() const{
		return table_;
	}

private:
	MyMergeTable table_;
};

struct MergeTableFactory_2{
	using DiskTable		= hm4::disk::DiskTable;
	using MyMergeTable	= hm4::multi::DualTable<DiskTable, DiskTable>;

	MergeTableFactory_2(const char *filename1, const char *filename2, int const advice){
		table1_.open(filename1, advice);
		table2_.open(filename2, advice);
	}

	const MyMergeTable &operator()() const{
		return table_;
	}

private:
	DiskTable	table1_;
	DiskTable	table2_;
	MyMergeTable	table_{ table2_, table1_ };
};


struct MergeTableFactory_N{
	using ArgTableLoader	= hm4::tableloader::ArgTableLoader;
	using MyMergeTable	= hm4::multi::CollectionTable<ArgTableLoader::container_type>;

	MergeTableFactory_N(int const table_count, const char **path, int const advice) :
					loader_( table_count, path, advice ),
					table_( *loader_ ) {}

	const MyMergeTable &operator()() const{
		return table_;
	}

private:
	ArgTableLoader	loader_;
	MyMergeTable	table_;
};



int const ADVICE = MMAPFile::SEQUENTIAL;

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

			MergeTableFactory_1 factory{ path[0], ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);
		}

	case 2:
		{
			std::cout
				<< "Merging two tables..."		<< '\n'
				<< '\t' << path[0]			<< '\n'
				<< '\t' << path[1]			<< '\n'
			;

			MergeTableFactory_2 factory{ path[0], path[1], ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);

		}

	default:
		{
			std::cout
				<< "Merging multiple tables..."		<< '\n'
			;

			MergeTableFactory_N factory{ table_count, path, ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);
		}
	}
}

