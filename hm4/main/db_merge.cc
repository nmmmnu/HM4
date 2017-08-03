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
int merge(const TABLE &table, const char *output_file, bool const keepTombstones){
	hm4::disk::FileBuilder df;

	df.build(output_file, table, keepTombstones, /* aligned */ true);

	return 0;
}

#include "tableloader/argtableloader.h"

#include "multi/dualtable.h"
#include "multi/collectiontable.h"

struct MergeTableFactory_1{
	using DiskTable = hm4::disk::DiskTable;
	using MyMergeTable = DiskTable;

	MyMergeTable operator()(const char *filename, int const advice){
		MyMergeTable table;
		table.open(filename, advice);

		std::cout
			<< "Merging (cleanup) single table..."	<< '\n'
			<< '\t' << filename			<< '\n'
		;

		return table;
	}
};

struct MergeTableFactory_2{
	using DiskTable		= hm4::disk::DiskTable;
	using MyMergeTable	= hm4::multi::DualTable<DiskTable, DiskTable>;

	MyMergeTable operator()(const char *filename1, const char *filename2, int const advice){
		table1_.open(filename1, advice);
		table2_.open(filename2, advice);

		// table 2 have precedence
		MyMergeTable table(table2_, table1_);

		std::cout
			<< "Merging two tables..."		<< '\n'
			<< '\t' << filename1			<< '\n'
			<< '\t' << filename2			<< '\n'
		;

		return table;
	}

private:
	DiskTable table1_;
	DiskTable table2_;
};

struct MergeTableFactory_N{
	using ArgTableLoader	= hm4::tableloader::ArgTableLoader;
	using MyMergeTable	= hm4::multi::CollectionTable<ArgTableLoader::container_type>;

	MyMergeTable operator()(int const table_count, const char **path, int const advice){
		loader_ = { table_count, path, advice };

		MyMergeTable table( *loader_ );

		std::cout
			<< "Merging multiple tables..."		<< '\n'
		;

		return table;
	}

private:
	ArgTableLoader loader_;
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
			MergeTableFactory_1 factory;

			const auto &table = factory(path[0], ADVICE);

			return merge(table, output, keepTombstones);
		}

	case 2:
		{
			MergeTableFactory_2 factory;

			const auto &table = factory(path[0], path[1], ADVICE);

			return merge(table, output, keepTombstones);

		}

	default:
		{
			MergeTableFactory_N factory;

			const auto &table = factory(table_count, path, ADVICE);

			return merge(table, output, keepTombstones);
		}
	}
}

