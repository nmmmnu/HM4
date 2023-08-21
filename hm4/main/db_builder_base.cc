#include <algorithm>

#include "version.h"

#include "logger.h"
#include "stringtokenizer.h"
#include "base64.h"

#include "blackholelist.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

constexpr size_t	MB			= 1024 * 1024;
constexpr size_t	PROCESS_STEP		= 1'000'000;
constexpr char		DELIMITER		= '\t';

constexpr size_t	BUFFER_SIZE		= 16 * 1024;

using MyReaderBuffer = std::array<char, BUFFER_SIZE>;



template <class List, bool Base64>
auto listInsert(List &list, std::string_view const key, std::string_view const val, std::bool_constant<Base64>){
	MyReaderBuffer base64_buffer;

	if constexpr(Base64)
		return insert(list, key, base64_decode(val, base64_buffer.data()));
	else
		return insert(list, key, val);
}


template <class List, bool Base64, bool InsertIgnore>
void listInsert(List &list, std::string_view const key, std::string_view const val, std::bool_constant<Base64> base64, std::bool_constant<InsertIgnore>){
	auto it = listInsert(list, key, val, base64);

	if constexpr(InsertIgnore)
		return;

	if ( it == std::end(list) )
		logger<Logger::WARNING>() << "Error insert" << key;
}



template <bool InsertIgnore, class List>
void printStats(List &list, size_t const count){
	auto const used = list.getAllocator().getUsedMemory();

	if constexpr(InsertIgnore){
		fmt::print(stderr,
			"Processed {:15} records.\n",
			count
		);
	}else if (used != std::numeric_limits<decltype(used)>::max()){
		fmt::print(stderr,
			"Processed {:15} records. "
			"In memory {:15} records, {:15} bytes. "
			"Allocator {:15} bytes.\n",
			count,
			list.size(), list.bytes(),
			used
		);
	}else{
		fmt::print(stderr,
			"Processed {:15} records. "
			"In memory {:15} records, {:15} bytes.\n",
			count,
			list.size(), list.bytes()
		);
	}
}



template <class List, class Reader, bool Base64, bool InsertIgnore>
int listLoad(List &list, Reader &reader, size_t const process_step, std::bool_constant<Base64> base64, std::bool_constant<InsertIgnore> insertIgnore){
	size_t i = 0;

	while(reader){
		// reader will keep line alive...
		std::string_view const line = reader.getLine();

		StringTokenizer const st{ line, DELIMITER };

		auto tok = getForwardTokenizer(st);

		std::string_view const key = tok();
		std::string_view const val = tok();

		if (! key.empty())
			listInsert(list, key, val, base64, insertIgnore);

		++i;

		if (i % process_step == 0)
			printStats<InsertIgnore>(list, i);
	}

	return 0;
}

int printUsage(std::string_view const cmd, std::string_view const reader_name){
	fmt::print(	"db_builder version {version}\n"
			"\n"
			"Build:\n"
			"\tDate   : {date} {time}\n"
			"\n"
			"\tReader : {reader_name}\n"
			"\tBuffer : {buffer_size}\n"
			"Usage:\n"
			"\t{cmd} [file.txt] [lsm_path] [memlist arena in MB] [b = import as base64 blobs] - load file.txt, then create / add to lsm_path\n"
			"\t\tPath names must be written with quotes:\n"
			"\t\t\tExample directory/file.'*'.db\n"
			"\t\t\tThe '*', will be replaced with ID's\n"
			"\t\tDo not overcommit memlist arena!\n"
			"\n",

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("date",	__DATE__		),
			fmt::arg("time",	__TIME__		),
			fmt::arg("cmd",		cmd			),
			fmt::arg("reader_name",	reader_name		),
			fmt::arg("buffer_size",	BUFFER_SIZE		)

	);

	return 10;
}

template<class MyReader>
int printUsage(std::string_view const cmd){
	return printUsage(cmd, MyReader::name());
}

template<class MyReader, class List, bool Base64, bool InsertIgnore>
int process(List &mylist, const char *filename, std::bool_constant<Base64> base64, std::bool_constant<InsertIgnore> insertIgnore){
	MyReaderBuffer reader_buffer;

	MyReader reader{ filename, reader_buffer.data(), reader_buffer.size() };

	return listLoad(mylist, reader, PROCESS_STEP, base64, insertIgnore);
}

template<class MyReader, class MyListFactory>
int process(MyListFactory &&factory, const char *filename, bool const base64){
	auto &mylist = factory();

	using InsertIgnore = std::is_same<
				typename MyListFactory::MemList,
				hm4::BlackHoleList
	>;

	logger<Logger::STARTUP>() << "Start building with" << MyListFactory::MemList::getName();

	if (base64)
		return process<MyReader>(mylist, filename, std::true_type{},  InsertIgnore{});
	else
		return process<MyReader>(mylist, filename, std::false_type{}, InsertIgnore{});
}


