#include "disk/disklist.h"

#include "version.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using hm4::disk::DiskList;

constexpr DiskList::size_type SAMPLES = 1024;

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_preload version {0}\n"
				"\n"
				"Usage:\n"
				"\t{1} [file] - preload {2} samples from file.db into system cache\n"
				"\n",
				hm4::version::str,
				cmd,
				SAMPLES
		);

		return 10;
	}

	int op_preload(DiskList const &list){
		auto const step = list.size() / SAMPLES;

		if (step)
			for (DiskList::size_type i = 0; i < list.size(); i+=step)
				fmt::print("{:>10} | {}\n", i, list[i].getKey() );

		return 0;
	}

} // namespace

int main(int argc, char **argv){
	if (argc <= 1)
		return printUsage(argv[0]);

	const char *filename	= argv[1];

	DiskList list;
	if (!list.open(filename, DiskList::NoVMAllocator{})){
		printf("Database file does not exists or is incorrect.\n");
		return 2;
	}

	list.printMetadata();

	return op_preload(list);
}

