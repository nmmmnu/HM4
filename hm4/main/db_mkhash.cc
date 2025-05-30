#include "disk/hashindexbuilder.h"

#include "version.h"

#include "mmapbuffer.h"

#include "disk/disklist.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using hm4::disk::hash::HashIndexBuilder;

using hm4::disk::DiskList;

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_mkhash version {version}\n"
				"\n"
				"Build:\n"
				"\tDate   : {date} {time}\n"
				"Usage:\n"
				"\t{cmd} [file.db] [hashindex_file.db] [arena in MB] - create hashmap for file.db\n"
				"\n",
				fmt::arg("version",	hm4::version::str	),
				fmt::arg("date",	__DATE__		),
				fmt::arg("time",	__TIME__		),
				fmt::arg("cmd",		cmd			)
		);

		return 10;
	}

} // namespace



constexpr size_t MIN_ARENA_SIZE	= 8;
constexpr size_t MB		= 1024 * 1024;

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	const char *input_file	= argv[1];
	const char *output_file	= argv[2];
	size_t const bufferSize = std::max(from_string<size_t>(argv[3]), MIN_ARENA_SIZE) * MB;

	DiskList list;
	if (!list.open(input_file, DiskList::NoVMAllocator{}, DiskList::OpenMode::FORWARD)){
		printf("Database file does not exists or is incorrect.\n");
		return 2;
	}

	list.printMetadata();

	MyBuffer::MMapMemoryResource buffer{ bufferSize };

	HashIndexBuilder builder{ output_file, list.size(), buffer };

	for(auto const &p : list)
		builder(p);
}

