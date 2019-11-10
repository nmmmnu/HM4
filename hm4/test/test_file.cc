#include <iostream>
#include <iomanip>
#include <type_traits>

#include "filereader.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "trackingallocator.h"
#include "arenaallocator.h"

constexpr size_t ARENA_SIZE = 1ULL * 1024 * 1024 * 1024;

using Allocator_s0 = MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;
using Allocator_s1 = MyAllocator::PMOwnerAllocator<MyAllocator::TrackingAllocator<MyAllocator::STDAllocator> >;
using Allocator_s  = Allocator_s0;
using Allocator_a  = MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;


Allocator_s allocator_s;
Allocator_a allocator_a{ ARENA_SIZE };

constexpr unsigned int PROCESS_STEP = 1000 * 10;

namespace{

	int printUsage(const char *cmd){
		std::cout
			<< "Usage:"	<< '\n'
			<< "\t"		<< cmd	<< " s [class] [file.txt] [key]     - load file.txt, then search for the key"		<< '\n'
			<< "\t"		<< cmd	<< " l [class] [file.txt] [key]     - load file.txt, then list using iterator"		<< '\n'
			<< '\n'

			<< "Classes are:"			<< '\n'
			<< '\t' << "v - VectorList"		<< '\n'
			<< '\t' << "l - LinkList"		<< '\n'
			<< '\t' << "s - SkipList"		<< '\n'
			<< '\t' << "V - VectorList (arena)"	<< '\n'
			<< '\t' << "L - LinkList   (arena)"	<< '\n'
			<< '\t' << "S - SkipList   (arena)"	<< '\n'

			<< '\n';

		return 10;
	}



	template <class LIST, class READER>
	void listLoad(LIST &list, READER &reader, bool const tombstones = true){
		typename LIST::size_type i = 0;

		while( reader ){
			std::string_view const key = reader.getLine();

			std::string_view const val = tombstones ? "" : key;

			if (! key.empty())
				list.insert(key, val);

			++i;

			if (i % PROCESS_STEP == 0){
				std::cout << "Processed "
						<< std::setw(10) << i			<< " records,"	<< ' '
						<< std::setw(10) << list.bytes()	<< " bytes."	<< '\n'
				;
			}
		}
	}

	template <class List>
	void listSearch(const List &list, std::string_view const key){
		auto const it = list.find(key, std::true_type{});

		if (it == std::end(list)){
			printf("Key '%s' not found...\n", key.data());
			return;
		}

		print(*it);
	}

	template <class LIST>
	void listIterate(const LIST &list, std::string_view const key, size_t count = 10){
		for(auto it = list.find(key, std::false_type{}); count && it != list.end(); ++it, --count)
			print(*it);
	}

	template <class LIST, class READER>
	int listSearchProcess(LIST &&list, READER &reader, std::string_view const key, bool const it){
		printf("Load start...\n");
		listLoad(list, reader);
		printf("Load done...\n");
		getchar();

		printf("Search start...\n");
		if (it){
			listIterate(list, key);
		}else{
			listSearch(list, key);
		}
		printf("Search done...\n");
		getchar();

		return 0;
	}

} // namespace

#include "vectorlist.h"
#include "linklist.h"
#include "skiplist.h"

namespace {

	int file_search(char const type, std::string_view const filename, std::string_view const key, bool const it){
		FileReader<4096> reader{ filename };

		switch(type){
		default:
		case 's':	return listSearchProcess(hm4::SkipList	 { allocator_s }, reader, key, it);
		case 'v':	return listSearchProcess(hm4::VectorList { allocator_s }, reader, key, it);
		case 'l':	return listSearchProcess(hm4::LinkList	 { allocator_s }, reader, key, it);

		case 'S':	return listSearchProcess(hm4::SkipList	 { allocator_a }, reader, key, it);
		case 'V':	return listSearchProcess(hm4::VectorList { allocator_a }, reader, key, it);
		case 'L':	return listSearchProcess(hm4::LinkList	 { allocator_a }, reader, key, it);
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

