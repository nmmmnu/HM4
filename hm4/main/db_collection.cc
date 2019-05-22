#include "db_file.h.cc"

#include "multi/collectionlist.h"
#include "listloader/directorylistloader.h"

#include <vector>
#include <iostream>

namespace{

	int printUsage(const char *cmd){
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

} // namespace

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	// =======================

	const char *op		= argv[1];
	const char *path	= argv[2];
	const char *key		= argv[3];

	// =======================

	using hm4::disk::DiskList;
	using Container		= std::vector<DiskList>;
	using MyListLoader	= hm4::listloader::DirectoryListLoader<Container>;
	using MyCollectionList	= hm4::multi::CollectionListFromIterator<Container::const_iterator>;

	Container		container;
	MyListLoader		loader{ container, path };
	MyCollectionList	list( std::begin(container), std::end(container) );

	// =======================

	switch(op[0]){
	case 'r':	return op_search (list, key);
	case 'l':	return op_iterate(list, key);
	}

	return printUsage(argv[0]);
}

