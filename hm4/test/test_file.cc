#include <iostream>
#include <iomanip>
#include <type_traits>

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "filereader.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "trackingallocator.h"
#include "arenaallocator.h"

constexpr size_t ARENA_SIZE = 6 * 1ULL * 1024 * 1024 * 1024;

using Allocator_std0	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;
using Allocator_std1	= MyAllocator::PMOwnerAllocator<MyAllocator::TrackingAllocator<MyAllocator::STDAllocator> >;
using Allocator_std	= Allocator_std0;
using Allocator_arena	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;
template<size_t Size>
using Allocator_arenaSt	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocatorStatic<Size> >;


Allocator_std			allocator_std;
Allocator_arena			allocator_arena{ ARENA_SIZE };
//Allocator_arenaSt<ARENA_SIZE>	allocator_arena;

constexpr unsigned int PROCESS_STEP = 1000 * 10;

#include "skiplist.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"Usage:\n"
				"\t{1}  s [class] [file.txt] [key] - load file.txt, then search for the key\n"
				"\t{1}  l [class] [file.txt] [key] - load file.txt, then list using iterator\n"
				"\n"
				"Classes are:\n",
				0,
				cmd
		);

		const char *format = "\t{} - {:10} {}\n";

		fmt::print(format, 'v', "VectorList"		, "std"		);
		fmt::print(format, 'l', "LinkList"		, "std"		);
		fmt::print(format, 's', "SkipList"		, "std"		);
		fmt::print(format, 'V', "VectorList"		, "arena"	);
		fmt::print(format, 'L', "LinkList"		, "arena"	);
		fmt::print(format, 'S', "SkipList"		, "arena"	);

		return 10;
	}



	template <class LIST, class READER>
	void listLoad(LIST &list, READER &reader, bool const tombstones = true){
		typename LIST::size_type i = 0;

		while( reader ){
			std::string_view const key = reader.getLine();

			std::string_view const val = tombstones ? "" : key;

			if (! key.empty())
				insert(list, key, val);

			++i;

			if (i % PROCESS_STEP == 0){
				fmt::print(
					"Processed {:10} records, {:10} bytes, {:10} allocator.\n",
					i,
					list.bytes(),
					list.getAllocator().getUsedMemory()
				);
			}
		}
	}

	template <class List>
	void listSearch(const List &list, std::string_view const key){
		auto const it = list.find(key, std::true_type{});

		if (it == std::end(list)){
			fmt::print("Key '{}' not found...\n", key);
			return;
		}

		print(*it);
	}

	template <class List>
	void listIterate(const List &list, std::string_view const key, size_t count = 10){
		for(auto it = list.find(key, std::false_type{}); count && it != list.end(); ++it, --count)
			print(*it);
	}

	template <class List, class Reader>
	int listSearchProcess(List &&list, Reader &reader, std::string_view const key, bool const it){
		puts("Load start...\n");
		listLoad(list, reader);
		puts("Load done...\n");
		getchar();

		puts("Search start...\n");
		if (it){
			listIterate(list, key);
		}else{
			listSearch(list, key);
		}
		puts("Search done...\n");
		getchar();

		if constexpr(std::is_same_v<List, hm4::SkipList<typename List::Allocator> >){
		//	list.printLanesSummary();
		}

		return 0;
	}

} // namespace

#include "vectorlist.h"
#include "linklist.h"

namespace {

	int file_search(char const type, std::string_view const filename, std::string_view const key, bool const it){
		const size_t buffer_size = 4096;

		char buffer[buffer_size];
		FileReader reader{ filename, buffer, buffer_size };

		using Allocator = MyAllocator::PMAllocator;

		switch(type){
		default:
		case 'v':	return listSearchProcess(hm4::VectorList	<Allocator>	{ allocator_std   }, reader, key, it);
		case 'l':	return listSearchProcess(hm4::LinkList	 	<Allocator>	{ allocator_std   }, reader, key, it);
		case 's':	return listSearchProcess(hm4::SkipList	 	<Allocator>	{ allocator_std   }, reader, key, it);

		case 'V':	return listSearchProcess(hm4::VectorList	<Allocator>	{ allocator_arena }, reader, key, it);
		case 'L':	return listSearchProcess(hm4::LinkList		<Allocator>	{ allocator_arena }, reader, key, it);
		case 'S':	return listSearchProcess(hm4::SkipList		<Allocator>	{ allocator_arena }, reader, key, it);
		}
	}

} // namespace

int main(int argc, char **argv){
	if (argc != 5)
		return printUsage(argv[0]);

	const char *op		= argv[1];
	const char *type	= argv[2];
	const char *filename	= argv[3];
	const char *key		= argv[4];

	switch(op[0]){
	case 's': return file_search( type[0], filename, key, false	);
	case 'l': return file_search( type[0], filename, key, true	);
	}

	return printUsage(argv[0]);
}

