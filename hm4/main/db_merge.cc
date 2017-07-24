#include "disk/filebuilder.h"

#include <unistd.h>	// access

static int printUsage(const char *name){
	const char *txt =
		"\t%s %c output_file [file1] [file2] [fileN...]"
		"- merge files, %-6s tombstones\n";

	printf("Usage:\n");
	printf(txt, 	name, '-', "keep");
	printf(txt, 	name, 't', "remove");
	printf("\t\tFiles must be written with the extention.\n");
	printf("\t\tExample 'file.db'\n");
	printf("\t\tDo not forget you usually need two output files\n");

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

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	bool keepTombstones	= argv[1][0] == 't' ? false : true;
	const char *output	= argv[2];

	const char **path	= (const char **) &argv[3];
	int const pathc		= argc - 3;

	if (fileExists(output)){
		printf("File %s exists. Please remove it and try again.\n", output);
		return 2;
	}

	using DiskTable = hm4::disk::DiskTable;

	if (pathc == 1){
		const char *filename = path[0];

		using MyMergeTable = DiskTable;

		MyMergeTable table;
		table.open(filename);

		printf("Merging (cleanup) single table...\n");
		printf("\t%s\n", filename);

		return merge(table, output, keepTombstones);

	}else if (pathc == 2){
		const char *filename1 = path[0];
		const char *filename2 = path[1];

		DiskTable table1;
		table1.open(filename1);

		DiskTable table2;
		table2.open(filename2);

		using MyMergeTable = hm4::multi::DualTable<DiskTable, DiskTable>;

		// table 2 have precedence
		MyMergeTable table(table2, table1);

		printf("Merging two tables...\n");
		printf("\t%s\n", filename1);
		printf("\t%s\n", filename2);

		return merge(table, output, keepTombstones);

	}else{
		using ArgTableLoader = hm4::tableloader::ArgTableLoader;

		ArgTableLoader al { pathc, path };

		using MyMergeTable = hm4::multi::CollectionTable<ArgTableLoader::container_type>;

		MyMergeTable table( *al );

		printf("Merging multiple tables...\n");

		return merge(table, output, keepTombstones);
	}
}

