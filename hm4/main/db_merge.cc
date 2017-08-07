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


#include "listloader/arglistloader.h"

#include "multi/dualtable.h"
#include "multi/collectionlist.h"

struct MergeListFactory_1{
	using DiskList = hm4::disk::DiskList;
	using MyMergeList = DiskList;

	MergeListFactory_1(const char *filename, int const advice){
		table_.open(filename, advice);
	}

	const MyMergeList &operator()() const{
		return table_;
	}

private:
	MyMergeList table_;
};

struct MergeListFactory_2{
	using DiskList		= hm4::disk::DiskList;
	using MyMergeList	= hm4::multi::DualTable<DiskList, DiskList>;

	MergeListFactory_2(const char *filename1, const char *filename2, int const advice){
		table1_.open(filename1, advice);
		table2_.open(filename2, advice);
	}

	const MyMergeList &operator()() const{
		return table_;
	}

private:
	DiskList	table1_;
	DiskList	table2_;
	MyMergeList	table_{ table2_, table1_ };
};


struct MergeListFactory_N{
	using ArgListLoader	= hm4::listloader::ArgListLoader;
	using MyMergeList	= hm4::multi::CollectionList<ArgListLoader::container_type>;

	MergeListFactory_N(int const table_count, const char **path, int const advice) :
					loader_( table_count, path, advice ),
					table_( *loader_ ) {}

	const MyMergeList &operator()() const{
		return table_;
	}

private:
	ArgListLoader	loader_;
	MyMergeList	table_;
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

			MergeListFactory_1 factory{ path[0], ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);
		}

	case 2:
		{
			std::cout
				<< "Merging two tables..."		<< '\n'
				<< '\t' << path[0]			<< '\n'
				<< '\t' << path[1]			<< '\n'
			;

			MergeListFactory_2 factory{ path[0], path[1], ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);

		}

	default:
		{
			std::cout
				<< "Merging multiple tables..."		<< '\n'
			;

			MergeListFactory_N factory{ table_count, path, ADVICE };

			return mergeFromFactory(factory, output, keepTombstones);
		}
	}
}

