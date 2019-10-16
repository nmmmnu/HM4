#include <iostream>
#include <iomanip>
#include <type_traits>

#include "filereader.h"

#include "pair.h"

constexpr unsigned int PROCESS_STEP = 1000 * 10;
using hm4::Pair;
using hm4::print;

namespace{

	int printUsage(const char *cmd){
		std::cout
			<< "Usage:"	<< '\n'
			<< "\t"		<< cmd	<< " s [class] [file.txt] [key]     - load file.txt, then search for the key"		<< '\n'
			<< "\t"		<< cmd	<< " l [class] [file.txt] [key]     - load file.txt, then list using iterator"		<< '\n'

			<< "\t\tPath names must be written like this:"		<< '\n'
			<< "\t\tExample 'directory/file.*.db'"			<< '\n'
			<< "\t\tThe '*', will be replaced with ID's"		<< '\n'

			<< '\n'

			<< "Classes are:"		<< '\n'
			<< '\t' << "v - VectorList"	<< '\n'
			<< '\t' << "l - LinkList"	<< '\n'
			<< '\t' << "s - SkipList"	<< '\n'

			<< '\n';

		return 10;
	}



	template <class LIST, class READER>
	void listLoad(LIST &list, READER &reader, bool const tombstones = true){
		typename LIST::size_type i = 0;

		while( reader ){
			std::string_view const key = std::string{ reader.getLine() };

			std::string_view const val = tombstones ? "" : key;

			if (! key.empty())
				list.insert( { key, val } );

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
		case 's':	return listSearchProcess(hm4::SkipList{},	reader, key, it);
		case 'v':	return listSearchProcess(hm4::VectorList{},	reader, key, it);
		case 'l':	return listSearchProcess(hm4::LinkList{},	reader, key, it);
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

