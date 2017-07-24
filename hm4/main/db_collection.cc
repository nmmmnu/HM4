#include "db_file_impl.h"

#include "multi/collectiontable.h"

#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " r [lsm_path] [key] - load lsm_path, then search for the key"	<< '\n'
		<< "\t"		<< cmd	<< " l [lsm_path] -     - load lsm_path, then list using iterator"	<< '\n'
		<< "\t"		<< cmd	<< " l [lsm_path] [key] - load lsm_path, then list using iterator"	<< '\n'

		<< "\t\tPath names must be written without extention"		<< '\n'
		<< "\t\tExample 'directory/file.*.db'"				<< '\n'

		<< '\n';

	return 10;
}

#include "tableloader/directorytableloader.h"

int main(int argc, char **argv){
	if (argc <= 3){
		printUsage(argv[0]);
		return 1;
	}

	// =======================

	const char *op		= argv[1];
	const char *path	= argv[2];
	const char *key		= argv[3];

	// =======================

	using MyTableLoader	= hm4::tableloader::DirectoryTableLoader;
	using MyCollectionTable	= hm4::multi::CollectionTable<MyTableLoader::container_type>;

	MyTableLoader		dl{ path };
	MyCollectionTable	list(*dl);

	// =======================

	switch(op[0]){
	case 'r':	return op_search (list, key);
	case 'l':	return op_iterate(list, key);
	}

	return printUsage(argv[0]);
}

