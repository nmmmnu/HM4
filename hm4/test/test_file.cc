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
#include "mmapbuffer.h"

constexpr size_t ARENA_SIZE = 1ULL * 1024 * 1024 * 1024 * 6;

using Allocator_std0	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;
using Allocator_std1	= MyAllocator::PMOwnerAllocator<MyAllocator::TrackingAllocator<MyAllocator::STDAllocator> >;
using Allocator_std	= Allocator_std0;

using ArenaBuffer	= MyBuffer::MMapBufferResource;
using Allocator_arena	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;

constexpr unsigned int PROCESS_STEP = 1000 * 10;

#include "skiplist.h"
#include "unrolledskiplist.h"
#include "unrolledlinklist.h"
#include "avllist.h"

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

		const char *format = "\t{} - {:20} {}\n";

		fmt::print(format, 'v', "VectorList"		, "std"		);
		fmt::print(format, 'l', "LinkList"		, "std"		);
		fmt::print(format, 's', "SkipList"		, "std"		);
		fmt::print(format, 'i', "UnrolledLinkList"	, "std"		);
		fmt::print(format, 'z', "UnrolledSkipList"	, "std"		);
		fmt::print(format, 'a', "AVLList"		, "std"		);
		fmt::print(format, 'V', "VectorList"		, "mmap arena"	);
		fmt::print(format, 'L', "LinkList"		, "mmap arena"	);
		fmt::print(format, 'S', "SkipList"		, "mmap arena"	);
		fmt::print(format, 'I', "UnrolledLinkList"	, "mmap arena"	);
		fmt::print(format, 'Z', "UnrolledskipList"	, "mmap arena"	);
		fmt::print(format, 'A', "AVLList"		, "mmap arena"	);

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

		if constexpr(std::is_same_v<List, hm4::UnrolledSkipList<typename List::Allocator> >){
		//	list.printLanesSummary();
		//	list.print();
		}

		if constexpr(std::is_same_v<List, hm4::UnrolledLinkList<typename List::Allocator> >){
		//	list.print();
		}

		if constexpr(std::is_same_v<List, hm4::AVLList<typename List::Allocator> >){
		//	list.testALVTreeIntegrity(std::false_type{});
		//	list.testALVTreeIntegrity(std::true_type{});
			fmt::print("AVL Tree height is {}.\n", list.height());
		}

		return 0;
	}

} // namespace

#include "vectorlist.h"
#include "linklist.h"

namespace {

	template<class TAllocator>
	int file_search(char const type, std::string_view const filename, std::string_view const key, bool const it, TAllocator &allocator){
		const size_t buffer_size = 4096;

		char buffer[buffer_size];
		FileReader reader{ filename, buffer, buffer_size };

		using Allocator = MyAllocator::PMAllocator;

		switch(type){
		default:
		case 'v':
		case 'V':	return listSearchProcess(hm4::VectorList	<Allocator>	{ allocator }, reader, key, it);

		case 'l':
		case 'L':	return listSearchProcess(hm4::LinkList		<Allocator>	{ allocator }, reader, key, it);

		case 's':
		case 'S':	return listSearchProcess(hm4::SkipList		<Allocator>	{ allocator }, reader, key, it);

		case 'i':
		case 'I':	return listSearchProcess(hm4::UnrolledLinkList	<Allocator>	{ allocator }, reader, key, it);

		case 'z':
		case 'Z':	return listSearchProcess(hm4::UnrolledSkipList	<Allocator>	{ allocator }, reader, key, it);

		case 'a':
		case 'A':	return listSearchProcess(hm4::AVLList		<Allocator>	{ allocator }, reader, key, it);
		}
	}

	int file_search(char const type, std::string_view const filename, std::string_view const key, bool const it){

		switch(type){
		default:
		case 'v':
		case 'l':
		case 's':
		case 'i':
		case 'z':
		case 'a': {
				Allocator_std	allocator;

				return file_search(type, filename, key, it, allocator);
			}

		case 'V':
		case 'L':
		case 'S':
		case 'I':
		case 'Z':
		case 'A': {
				ArenaBuffer	arena_buffer{ ARENA_SIZE };
				Allocator_arena	allocator{ arena_buffer };

				return file_search(type, filename, key, it, allocator);
			}
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

	if (key[0] == '\0'){
		fmt::print("Key can not be empty.\n");
		return -100;
	}

	switch(op[0]){
	case 's': return file_search( type[0], filename, key, false	);
	case 'l': return file_search( type[0], filename, key, true	);
	}

	return printUsage(argv[0]);
}

