#include "db_file_impl.h"

#include "disk/disklist.h"

#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " r [file.db] [key] - load file.db, then search for the key"		<< '\n'
		<< "\t"		<< cmd	<< " l [file.db] -     - load file.db, then list using iterator"	<< '\n'
		<< "\t"		<< cmd	<< " l [file.db] [key] - load file.db, then list using iterator"	<< '\n'

		<< "\t\tPath names must be written like this:"	<< '\n'
		<< "\t\tExample 'directory/file.db'"		<< '\n'

		<< '\n';

	return 10;
}

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	// =======================

	const char *op		= argv[1];
	const char *filename	= argv[2];
	const char *key		= argv[3];

	// =======================

	using DiskTable = hm4::disk::DiskList;

	DiskTable list;
	if (list.open(filename) == false){
		printf("Database file does not exists or is incorrect.\n");
		return 2;
	}

	list.printMetadata();

	// =======================

	switch(op[0]){
	case 'r':	return op_search (list, key);
	case 'l':	return op_iterate(list, key);
	}

	return printUsage(argv[0]);
}

