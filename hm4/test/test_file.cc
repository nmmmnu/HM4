#include <iostream>
#include <iomanip>
#include <type_traits>

#include "filereader.h"
#include "disk/filebuilder.h"

#include "pair.h"

constexpr unsigned int PROCESS_STEP = 1000 * 10;
using hm4::Pair;
using hm4::print;



static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " s [class] [file.txt] [key]     - load file.txt, then search for the key"		<< '\n'
		<< "\t"		<< cmd	<< " l [class] [file.txt] [key]     - load file.txt, then list using iterator"		<< '\n'
		<< "\t"		<< cmd	<< " w [class] [file.txt] [file.db] - load file.txt, then save it under file.db"	<< '\n'

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
static void listLoad(LIST &list, READER &reader, bool const tombstones = true){
	typename LIST::size_type i = 0;

	while( reader ){
		std::string line = reader.getLine();

		const StringRef key = line;
		const StringRef val = tombstones ? nullptr : key;

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
static void listSearch(const List &list, const StringRef &key){
	auto const it = list.find(key, std::true_type{});

	if (it == std::end(list)){
		printf("Key '%s' not found...\n", key.data());
		return;
	}

	print(*it);
}

template <class LIST>
static void listIterate(const LIST &list, const StringRef &key, size_t count = 10){
	for(auto it = list.find(key, std::false_type{}); count && it != list.end(); ++it, --count)
		print(*it);
}

template <class LIST, class READER>
static int listSearchProcess(LIST &&list, READER &reader, const StringRef &key, bool const it){
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

template <class LIST, class READER>
static int listWriteProcess(LIST &&list, READER &reader, const StringRef &filename2){
	printf("Load start...\n");
	listLoad(list, reader);
	printf("Load done...\n");
	getchar();

	printf("Write start...\n");
	hm4::disk::FileBuilder::build(filename2, list, /* keep tombstones */ true, /*aligned */ true);
	printf("Write done...\n");
	getchar();

	return 0;
}

#include "vectorlist.h"
#include "linklist.h"
#include "skiplist.h"

static int file_search(char const type, const StringRef &filename, const StringRef &key, bool const it){
	FileReader<4096> reader{ filename };

	switch(type){
	default:
	case 's':	return listSearchProcess(hm4::SkipList{},	reader, key, it);
	case 'v':	return listSearchProcess(hm4::VectorList{},	reader, key, it);
	case 'l':	return listSearchProcess(hm4::LinkList{},	reader, key, it);
	}
}

static int file_write(char const type, const StringRef &filename, const StringRef &filename2){
	FileReader<4096> reader{ filename };

	switch(type){
	default:
	case 's':	return listWriteProcess(hm4::SkipList{},	reader, filename2);
	case 'v':	return listWriteProcess(hm4::VectorList{},	reader, filename2);
	case 'l':	return listWriteProcess(hm4::LinkList{},	reader, filename2);
	}
}


int main(int argc, char **argv){
	if (argc != 5)
		return printUsage(argv[0]);

	const char *op		= argv[1];
	const char *type	= argv[2];
	const char *filename	= argv[3];
	const char *key		= argv[4];
	const char *filename2	= argv[4];

	switch(op[0]){
	case 's': return file_search( type[0], filename, key, false	);
	case 'l': return file_search( type[0], filename, key, true	);
	case 'w': return file_write(  type[0], filename, filename2	);
	}

	return printUsage(argv[0]);
}

