/* if removed, will build only file based stuff - for debug only */
#define BUILD_BOTH

#ifdef BUILD_BOTH

#include "multi/collectionlist.h"
#include "listloader/directorylistloader.h"

#endif

#include "disk/disklist.h"

#include "version.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_file version {0}\n"
				"\n"
				"Usage:\n"
				"\t{1} r [file.db] [key] - load file.db, then search for the key\n"
				"\t{1} l [file.db] -     - load file.db, then list using iterator\n"
				"\t{1} i [file.db] [key] - load file.db, then list using index\n"
				"\n",
				hm4::version::str,
				cmd
		);

		return 10;
	}

	template <class List>
	int op_search(List const &list, std::string_view const key){
		if (key.empty())
			return 1;

		if (const auto *p = list.getPair(key); p)
			print(*p);

		return 0;
	}

	template <class List>
	int op_iterate(List const &list, std::string_view const key, size_t const count = 10){
		size_t c = 0;

		auto it = key == "-" ? std::begin(list) : list.find(key);

		for(; it != std::end(list); ++it){
			using hm4::print;

			print(*it);

			if (++c >= count)
				break;
		}

		return 0;
	}


	template <class List>
	int op_id(List const &, std::string_view const ){
		return 2;
	}

	int op_id(hm4::disk::DiskList const &list, std::string_view const key){
		if (key.empty())
			return 1;

		auto const index = from_string<size_t>(key);

		if (index < list.size())
			print(list[index]);

		return 0;
	}

	template <class List>
	int op_select(char const op, List const &list, std::string_view const key){
		switch(op){
		default:
		case 'r': return op_search (list, key);
		case 'l': return op_iterate(list, key);
		case 'i': return op_id     (list, key);
		}
	}

} // namespace

int main(int argc, char **argv){
	if (argc <= 3)
		return printUsage(argv[0]);

	// =======================

	const char *op		= argv[1];
	const char *filename	= argv[2];
	const char *key		= argv[3];

	// =======================

#ifdef BUILD_BOTH

	using DirectoryListLoader = hm4::listloader::DirectoryListLoader;

	if (DirectoryListLoader::checkIfLoaderNeed(filename) == false){

#endif
		using hm4::disk::DiskList;

		DiskList list;
		if (!list.open(filename, DiskList::NoVMAllocator{})){
			printf("Database file does not exists or is incorrect.\n");
			return 2;
		}

		list.printMetadata();

		return op_select(op[0], list, key);

#ifdef BUILD_BOTH

	}else{
		using MyListLoader = DirectoryListLoader;

		MyListLoader	loader{ filename, MyListLoader::NoSlabAllocator{} };

		auto const 	&list = loader.getList();

		return op_select(op[0], list, key);
	}

#endif

	return printUsage(argv[0]);
}

