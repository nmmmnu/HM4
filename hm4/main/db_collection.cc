#include "db_file.h.cc"

#include "multi/collectionlist.h"
#include "listloader/directorylistloader.h"

#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " r [lsm_path] [key] - load lsm_path, then search for the key"	<< '\n'
		<< "\t"		<< cmd	<< " l [lsm_path] -     - load lsm_path, then list using iterator"	<< '\n'
		<< "\t"		<< cmd	<< " l [lsm_path] [key] - load lsm_path, then list using iterator"	<< '\n'

		<< "\t\tPath names must be written like this:"	<< '\n'
		<< "\t\tExample 'directory/file.*.db'"		<< '\n'

		<< '\n';

	return 10;
}

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	// =======================

	const char *op		= argv[1];
	const char *path	= argv[2];
	const char *key		= argv[3];

	// =======================

	using MyListLoader	= hm4::listloader::DirectoryListLoader;
	using MyCollectionList	= hm4::multi::CollectionListFromIterator<MyListLoader::iterator>;

	MyListLoader		dl{ path };
	MyCollectionList	list( std::begin(dl), std::end(dl) );

	// =======================

	switch(op[0]){
	case 'r':	return op_search (list, key);
	case 'l':	return op_iterate(list, key);
	}

	return printUsage(argv[0]);
}

