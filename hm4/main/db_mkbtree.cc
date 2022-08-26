#include "disk/btreeindexbuilder.h"

#include "disk/disklist.h"

#include "version.h"
#include "myfs.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_mkbtree version {0} \n"
				"\n"
				"Usage:\n"
				"\t{1} [file.db] [btree_file.db]\n"
				"\t\tDo not forget that usually [btree_file.db] must have same name as [file.db]\n"
				"\n",
				hm4::version::str,
				cmd
		);

		return 10;
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
		fmt::print("File {} exists. Please remove it and try again.\n", output);
		return 2;
	}
#else
	(void)fileExists;
#endif
	using DiskList = hm4::disk::DiskList;

	DiskList list;

	if ( list.open(input_file) ){
		fmt::print("List is invalid. Please check it and try again.\n");
		return 3;
	}

	using BTreeIndexBuilder = hm4::disk::btree::BTreeIndexBuilder<DiskList>;

	BTreeIndexBuilder builder(output_file, list);

	bool const result = builder.build();

	return result ? 0 : 1;
}

