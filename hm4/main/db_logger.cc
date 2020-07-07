#include "version.h"

#include "blackholelist.h"
#include "binlogger/diskfilebinlogger.h"
#include "binloglist.h"

#include "filereader.h"
#include "stringtokenizer.h"

#include "stdallocator.h"

#include "logger.h"

#include "base64.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using MySTDAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

constexpr size_t	PROCESS_STEP		= 10'000;

constexpr char		DELIMITER		= '\t';

using MyReader		= FileReader;

using MyReaderBuffer	= std::array<char, 16 * 1024>;

MyReaderBuffer	reader_buffer, base64_buffer;



static int printUsage(const char *cmd);



template <class LIST, class READER, bool B>
int listLoad(LIST &list, READER &reader, size_t const process_step, std::bool_constant<B> tag);



struct MyListFactory{
	using MemList	= hm4::BlackHoleList;
	using BinLogger	= hm4::binlogger::DiskFileBinLogger;
	using MyList	= hm4::BinLogList<MemList,BinLogger>;

	MyListFactory(std::string_view const filename, bool const aligned) :
				mylist{
					memlist,
					BinLogger{ filename, aligned }
				}{}

	MyList &operator()(){
		return mylist;
	}

private:
	MySTDAllocator	allocator;
	MemList		memlist{ allocator };
	MyList		mylist;
};


int main(int argc, char **argv){
	if (argc <= 2)
		return printUsage(argv[0]);

	const char *filename	= argv[1];
	const char *path	= argv[2];

	bool const blob = argc >= 4 && argv[3][0] == 'b';

	bool const aligned = true;

	MyListFactory factory{ path, aligned };

	auto &mylist = factory();

	MyReader reader{ filename, reader_buffer.data(), reader_buffer.size() };

	if (blob == false)
		return listLoad(mylist, reader, PROCESS_STEP, std::false_type{});
	else
		return listLoad(mylist, reader, PROCESS_STEP, std::true_type{});
}



template <class LIST, bool B>
auto listInsert(LIST &list, std::string_view const key, std::string_view const val, std::bool_constant<B>){
	if constexpr(B)
		return list.insert(key, base64_decode(val, base64_buffer.data()));
	else
		return list.insert(key, val);
}

template <class LIST, class READER, bool B>
int listLoad(LIST &list, READER &reader, size_t const process_step, std::bool_constant<B> tag){
	size_t i = 0;

	while(reader){
		// reader will keep line alive...
		std::string_view const line = reader.getLine();

		StringTokenizer const st{ line, DELIMITER };

		auto tok = getForwardTokenizer(st);

		std::string_view const key = tok();
		std::string_view const val = tok();

		if (! key.empty())
			listInsert(list, key, val, tag);

		++i;

		if (i % process_step == 0){
			fmt::print(stderr,
				"Processed {:15} records.\n",
				i
			);
		}
	}

	return 0;
}



static int printUsage(const char *cmd){
	fmt::print(	"db_logger version {0}\n"
			"\n"
			"Usage:\n"
			"\t{1} [file.txt] [binlog_filename] [b = import as base64 blobs] - load file.txt, then create binlog_file\n"
			"\n"
			"\tReader: {2}\n"
			"\tBuffer: {3}\n"
			"\n",
			hm4::version::str,
			cmd,
			MyReader::name(),
			reader_buffer.size()
	);

	return 10;
}


