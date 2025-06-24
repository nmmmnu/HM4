#include "disk/filebuilder.h"

#include "version.h"

#include "myfs.h"

#include "mmapbuffer.h"
#include "staticbuffer.h"

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
	int mergeFromFactory(FACTORY &&f, const char *output_file, hm4::disk::FileBuilder::TombstoneOptions const tombstoneOptions,
				hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite,
				MyBuffer::ByteBufferView bufferHash){

		auto const aligned = hm4::Pair::WriteOptions::ALIGNED;

		auto &list = f();

		if (bufferHash){
			hm4::disk::FileBuilder::build(output_file, buffersWrite,
								std::begin(list), std::end(list),
								tombstoneOptions, aligned,
								list.size(), bufferHash
			);

			return 0;
		}else{
			hm4::disk::FileBuilder::build(output_file, buffersWrite,
								std::begin(list), std::end(list),
								tombstoneOptions, aligned
			);

			return 0;
		}
	}

} // anonymous namespace



#include "listloader/iteratorlistloader.h"

#include "multi/duallist.h"
#include "multi/collectionlist.h"

using hm4::disk::DiskList;

struct MergeListFactory_1{
	MergeListFactory_1(const char *filename, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode){
		table_.open(filename, allocator, mode);
	}

	const auto &operator()() const{
		return table_;
	}

private:
	DiskList table_;
};



struct MergeListFactory_2{
	MergeListFactory_2(const char *filename1, const char *filename2, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode){
		table1_.open(filename1, allocator, mode);
		table2_.open(filename2, allocator, mode);
	}

	const auto &operator()() const{
		return table_;
	}

private:
	using MyDualList = hm4::multi::DualList<const DiskList, const DiskList, hm4::multi::DualListEraseType::NONE>;

	DiskList	table1_;
	DiskList	table2_;
	MyDualList	table_{ table1_, table2_ };
};



template<class IT>
struct MergeListFactory_N{
	MergeListFactory_N(IT first, IT last, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode) :
					loader_(first, last, allocator, mode){}

	const auto &operator()() const{
		return loader_.getList();
	}

private:
	hm4::listloader::IteratorListLoader<IT>	loader_;
};



constexpr auto	DEFAULT_MODE		= DiskList::OpenMode::FORWARD;

constexpr size_t MIN_HASH_ARENA_SIZE	= 8;
constexpr size_t MB			= 1024 * 1024;

// not to have 64K on the stack
MyBuffer::StaticMemoryResource<32 * 2048> g_slabBuffer;

#include "disk/filebuilder.misc.h"
// defines g_fbwb;

int main(int argc, char **argv){
	if (argc <= 1 + 1 + 1)
		return printUsage(argv[0]);

	const char *output = argv[2];

	if (fileExists(output)){
		fmt::print("File {} exists. Please remove it and try again.\n", output);
		return 2;
	}

	size_t const arenaHashSize_	= from_string<size_t>(argv[3]);
	size_t const arenaHashSize__  	= arenaHashSize_ < MIN_HASH_ARENA_SIZE ? 0 : arenaHashSize_;
	size_t const arenaHashSize 	= arenaHashSize__ * MB;

	MyBuffer::MMapMemoryResource bufferHash{ arenaHashSize };

	using hm4::disk::FileBuilder::TombstoneOptions;

	TombstoneOptions const tombstoneOptions = argv[1][0] == 't' ? TombstoneOptions::REMOVE : TombstoneOptions::KEEP;

	int const table_count	= argc - 4;
	const char **path	= (const char **) &argv[4];

	DiskList::VMAllocator allocator{ g_slabBuffer };

	auto buffersWrite = g_fbwb();

	switch(table_count){
	case 1:
		fmt::print(	"Merging (cleanup) single table...\n"
				"\t{}\n",
				path[0]
		);

		return mergeFromFactory(
			MergeListFactory_1{ path[0], allocator, DEFAULT_MODE },
			output,
			tombstoneOptions,
			buffersWrite,
			bufferHash
		);

	case 2:
		fmt::print(	"Merging two tables...\n"
				"\t{}\n"
				"\t{}\n",
				path[0], path[1]
		);

		return mergeFromFactory(
			MergeListFactory_2{ path[0], path[1], allocator, DEFAULT_MODE },
			output,
			tombstoneOptions,
			buffersWrite,
			bufferHash
		);

	default:
		fmt::print(	"Merging multiple tables...\n");

		return mergeFromFactory(
			MergeListFactory_N<const char **>{ path, path + table_count, allocator, DEFAULT_MODE },
			output,
			tombstoneOptions,
			buffersWrite,
			bufferHash
		);
	}
}

