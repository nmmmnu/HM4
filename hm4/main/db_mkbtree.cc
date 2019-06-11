#include "disk/btreeindexbuilder.h"

#include "disk/disklist.h"

#include "version.h"

#include <unistd.h>	// access

#include <iostream>

namespace{

	int printUsage(const char *cmd){
		std::cout
			<< "db_mkbtree version " << hm4::version::str 						<< '\n'
			<< '\n'

			<< "Usage:"	<< '\n'
			<< '\t'		<< cmd	<< "[file.db] [btree_file.db]"					<< '\n'

			<< "\t\tDo not forget that usually [btree_file.db] must have same name as [file.db]"	<< '\n'

			<< '\n';

		return 10;
	}

	inline bool fileExists(const StringRef& name) {
		return access(name.data(), F_OK) != -1;
	}

} // namespace

int main(int argc, char **argv){
	if (argc <= 1 + 1){
		printUsage(argv[0]);
		return 1;
	}

	const char *input_file	= argv[1];
	const char *output_file	= argv[2];
#if 0
	if (fileExists(output_file)){
		printf("File %s exists. Please remove it and try again.\n", output_file);
		return 2;
	}
#else
	(void)fileExists;
#endif
	using DiskList = hm4::disk::DiskList;

	DiskList list;
	list.open(input_file);

	if (list == false){
		printf("List is invalid. Please check it and try again.\n");
		return 3;
	}

	using BTreeIndexBuilder = hm4::disk::btree::BTreeIndexBuilder<DiskList>;

	BTreeIndexBuilder builder(output_file, list);

	bool const result = builder.build();

	return result ? 0 : 1;
}

