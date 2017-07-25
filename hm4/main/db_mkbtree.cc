#include "disk/btreeindexbuilder.h"

#include "disk/disktable.h"

#include <unistd.h>	// access


static int printUsage(const char *name){
	printf("Usage:\n");
	printf("%s [file.db] [btree_file.db]\n", name);
	printf("\t\tFiles must be written with the extention.\n");
	printf("\t\tExample 'file.db'\n");
	printf("\t\tDo not forget that usually btree_file must have same name as [file.db]\n");

	return 10;
}

inline bool fileExists(const StringRef& name) {
	return access(name.data(), F_OK) != -1;
}

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
#endif
	using DiskTable = hm4::disk::DiskTable;

	DiskTable list;
	list.open(input_file);

	using BTreeIndexBuilder = hm4::disk::btree::BTreeIndexBuilder<DiskTable>;

	BTreeIndexBuilder builder(output_file, list);

	bool const result = builder.build();

	return result ? 0 : 1;
}

