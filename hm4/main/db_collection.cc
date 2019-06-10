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

	using MyListLoader	= hm4::listloader::DirectoryListLoader;

	MyListLoader		loader{ path };

	// =======================

	switch(op[0]){
	case 'r':	return op_search (loader.getList(), key);
	case 'l':	return op_iterate(loader.getList(), key);
	}

	return printUsage(argv[0]);
}

