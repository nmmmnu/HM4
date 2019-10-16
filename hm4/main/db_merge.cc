#include "disk/filebuilder.h"

#include <unistd.h>	// access

#include "version.h"

#include <iostream>

namespace{

	int printUsage(const char *cmd){
		std::cout
			<< "db_merge version " << hm4::version::str 									<< '\n'
			<< '\n'

			<< "Usage:"	<< '\n'
			<< '\t'		<< cmd	<< " - output.db [file1.db] [file2.db] [fileN.db] - merge files, keep   tombstones"	<< '\n'
			<< '\t'		<< cmd	<< " t output.db [file1.db] [file2.db] [fileN.db] - merge files, remove tombstones"	<< '\n'

			<< "\t\tDo not forget you usually need two input files"	<< '\n'

			<< '\n';

		return 10;
	}

	inline bool fileExists(std::string_view const name) {
		return access(name.data(), F_OK) != -1;
	}

	template <class FACTORY>
	int mergeFromFactory(const FACTORY &f, const char *output_file, bool const keepTombstones){
		hm4::disk::FileBuilder::build(output_file, std::begin(f.getList()), std::end(f.getList()), keepTombstones, /* aligned */ true);

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

	const auto &getList() const{
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

	const auto &getList() const{
		return table_;
	}

private:
	using MyDualList = hm4::multi::DualList<const DiskList, const DiskList>;

	DiskList	table1_;
	DiskList	table2_;
	MyDualList	table_{ table2_, table1_ };
};



template<class IT>
struct MergeListFactory_N{
	MergeListFactory_N(IT first, IT last, const MMAPFile::Advice advice, DiskList::OpenMode const mode) :
					loader_(first, last, advice, mode){}

	const auto &getList() const{
		return loader_.getList();
	}

private:
	hm4::listloader::IteratorListLoader<IT>	loader_;
};



constexpr auto	DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto	DEFAULT_MODE	= DiskList::OpenMode::MINIMAL;

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	const char *output	= argv[2];

	if (fileExists(output)){
		printf("File %s exists. Please remove it and try again.\n", output);
		return 2;
	}

	bool const keepTombstones = argv[1][0] == 't' ? false : true;

	int const table_count	= argc - 3;
	const char **path	= (const char **) &argv[3];


	switch(table_count){
	case 1:
		{
			std::cout
				<< "Merging (cleanup) single table..."	<< '\n'
				<< '\t' << path[0]			<< '\n'
			;

			MergeListFactory_1 factory{ path[0], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);
		}

	case 2:
		{
			std::cout
				<< "Merging two tables..."		<< '\n'
				<< '\t' << path[0]			<< '\n'
				<< '\t' << path[1]			<< '\n'
			;

			MergeListFactory_2 factory{ path[0], path[1], DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);

		}

	default:
		{
			std::cout
				<< "Merging multiple tables..."		<< '\n'
			;

			MergeListFactory_N<const char **> factory{ path, path + table_count, DEFAULT_ADVICE, DEFAULT_MODE };

			return mergeFromFactory(factory, output, keepTombstones);
		}
	}
}

