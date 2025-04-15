#include "disk/filebuilder.h"

#include "version.h"

#include "myfs.h"

#include "mmapbuffer.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_merge version {version} \n"
				"\n"
				"Build:\n"
				"\tDate   : {date} {time}\n"
				"\n"
				"Usage:\n"
				"\t{cmd} - output.db [hash arena in MB] [file1.db] [file2.db] [fileN.db] - merge files, keep   tombstones\n"
				"\t{cmd} t output.db [hash arena in MB] [file1.db] [file2.db] [fileN.db] - merge files, remove tombstones\n"
				"\t\tDo not forget you usually need two input files\n"
				"\t\tIf hash arena in MB is zero, no hashindex will be made\n"
				"\n",
				fmt::arg("version",	hm4::version::str	),
				fmt::arg("date",	__DATE__		),
				fmt::arg("time",	__TIME__		),
				fmt::arg("cmd",		cmd			)
		);

		return 10;
	}

	template <class FACTORY>
	int mergeFromFactory(FACTORY &&f, const char *output_file, hm4::disk::FileBuilder::TombstoneOptions const tombstoneOptions, size_t const bufferSize){
		auto const aligned = hm4::Pair::WriteOptions::ALIGNED;

		auto &list = f();

		if (bufferSize){
			MyBuffer::ByteMMapBuffer bufferHash{ bufferSize };

			hm4::disk::FileBuilder::build(output_file, std::begin(list), std::end(list),
								tombstoneOptions, aligned,
								list.size(), bufferHash
			);
		}else{
			hm4::disk::FileBuilder::build(output_file, std::begin(list), std::end(list),
								tombstoneOptions, aligned
			);
		}

		return 0;
	}

} // anonymous namespace



#include "listloader/iteratorlistloader.h"

#include "multi/duallist.h"
#include "multi/collectionlist.h"

using hm4::disk::DiskList;

struct MergeListFactory_1{
	MergeListFactory_1(const char *filename, MMAPFile::Advice const advice, DiskList::OpenMode const mode){
		table_.open(filename, advice, mode);
	}

	const auto &operator()() const{
		return table_;
	}

private:
	DiskList table_;
};



struct MergeListFactory_2{
	MergeListFactory_2(const char *filename1, const char *filename2, const MMAPFile::Advice advice, DiskList::OpenMode const mode){
		table1_.open(filename1, advice, mode);
		table2_.open(filename2, advice, mode);
	}

	const auto &operator()() const{
		return table_;
	}

private:
	using MyDualList = hm4::multi::DualList<const DiskList, const DiskList, hm4::multi::DualListEraseType::NONE>;

	DiskList	table1_;
	DiskList	table2_;
	MyDualList	table_{ table2_, table1_ };
};



template<class IT>
struct MergeListFactory_N{
	MergeListFactory_N(IT first, IT last, const MMAPFile::Advice advice, DiskList::OpenMode const mode) :
					loader_(first, last, advice, mode){}

	const auto &operator()() const{
		return loader_.getList();
	}

private:
	hm4::listloader::IteratorListLoader<IT>	loader_;
};



constexpr auto	DEFAULT_ADVICE		= MMAPFile::Advice::SEQUENTIAL;
constexpr auto	DEFAULT_MODE		= DiskList::OpenMode::FORWARD;

constexpr size_t MIN_HASH_ARENA_SIZE	= 8;
constexpr size_t MB			= 1024 * 1024;

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	const char *output = argv[2];

	if (fileExists(output)){
		fmt::print("File {} exists. Please remove it and try again.\n", output);
		return 2;
	}

	size_t const hash_arena_ = from_string<size_t>(argv[3]);
	size_t const hash_arena  = hash_arena_ < MIN_HASH_ARENA_SIZE ? 0 : hash_arena_ * MB;

	using hm4::disk::FileBuilder::TombstoneOptions;

	TombstoneOptions const tombstoneOptions = argv[1][0] == 't' ? TombstoneOptions::REMOVE : TombstoneOptions::KEEP;

	int const table_count	= argc - 4;
	const char **path	= (const char **) &argv[4];

	switch(table_count){
	case 1:
		fmt::print(	"Merging (cleanup) single table...\n"
				"\t{}\n",
				path[0]
		);

		return mergeFromFactory(
			MergeListFactory_1{ path[0], DEFAULT_ADVICE, DEFAULT_MODE },
			output,
			tombstoneOptions,
			hash_arena
		);

	case 2:
		fmt::print(	"Merging two tables...\n"
				"\t{}\n"
				"\t{}\n",
				path[0], path[1]
		);

		return mergeFromFactory(
			MergeListFactory_2{ path[0], path[1], DEFAULT_ADVICE, DEFAULT_MODE },
			output,
			tombstoneOptions,
			hash_arena
		);

	default:
		fmt::print(	"Merging multiple tables...\n");

		return mergeFromFactory(
			MergeListFactory_N<const char **>{ path, path + table_count, DEFAULT_ADVICE, DEFAULT_MODE },
			output,
			tombstoneOptions,
			hash_arena
		);
	}
}

