#include "disk/filebuilder.h"

#include "version.h"

#include "myfs.h"

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_merge version {0} \n"
				"\n"
				"Usage:\n"
				"\t{1} - output.db [file1.db] [file2.db] [fileN.db] - merge files, keep   tombstones\n"
				"\t{1} t output.db [file1.db] [file2.db] [fileN.db] - merge files, remove tombstones\n"
				"\t\tDo not forget you usually need two input files\n"
				"\n",
				hm4::version::str,
				cmd
		);

		return 10;
	}

	template <class FACTORY>
	int mergeFromFactory(const FACTORY &f, const char *output_file, hm4::disk::FileBuilder::TombstoneOptions const tombstoneOptions){
		hm4::disk::FileBuilder::build(output_file, std::begin(f()), std::end(f()), tombstoneOptions, hm4::Pair::WriteOptions::ALIGNED);

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
	using MyDualList = hm4::multi::DualList<const DiskList, const DiskList, hm4::multi::DualListEraseType::NORMAL>;

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



constexpr auto	DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto	DEFAULT_MODE	= DiskList::OpenMode::FORWARD;

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	const char *output	= argv[2];

	if (fileExists(output)){
		fmt::print("File {} exists. Please remove it and try again.\n", output);
		return 2;
	}

	using hm4::disk::FileBuilder::TombstoneOptions;

	TombstoneOptions const tombstoneOptions = argv[1][0] == 't' ? TombstoneOptions::NONE : TombstoneOptions::KEEP;

	int const table_count	= argc - 3;
	const char **path	= (const char **) &argv[3];


	switch(table_count){
	case 1:
		{
			fmt::print(	"Merging (cleanup) single table...\n"
					"\t{}\n",
					path[0]
			);

			MergeListFactory_1 factory{ path[0], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, tombstoneOptions);
		}

	case 2:
		{
			fmt::print(	"Merging two tables...\n"
					"\t{}\n"
					"\t{}\n",
					path[0], path[1]
			);

			MergeListFactory_2 factory{ path[0], path[1], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, tombstoneOptions);

		}

	default:
		{
			fmt::print(	"Merging multiple tables...\n");

			MergeListFactory_N<const char **> factory{ path, path + table_count, DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, tombstoneOptions);
		}
	}
}

