#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "version.h"

#include "db_compact_options.h"

#include "myglob.h"
#include "stringreplace.h"
#include "idgenerator.h"
#include "disk/disklist.h"
#include "disk/filebuilder.h"

#include "listloader/iteratorlistloader.h"

#include "multi/duallist.h"
#include "multi/collectionlist.h"

#include "mmapbuffer.h"
#include "staticbuffer.h"

#include <vector>
#include <utility>
#include <algorithm>
#include "myspan.h"



using hm4::disk::DiskList;

constexpr auto DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto DEFAULT_MODE	= DiskList::OpenMode::FORWARD;

using IDGenerator = idgenerator::IDGeneratorDate;

using hm4::disk::FileBuilder::TombstoneOptions;

constexpr TombstoneOptions TOMBSTONE_OPTION = TombstoneOptions::KEEP;

// not to have 64K on the stack
MyBuffer::StaticMemoryResource<32 * 2048> g_slabBuffer;



namespace{

	MyOptions prepareOptions(int argc, char **argv);

	int compact(MyOptions const &opt, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferHash);

	template <class Factory>
	void mergeFromFactory(Factory &&f, std::string_view output_file, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferHash){
		auto const aligned = hm4::Pair::WriteOptions::ALIGNED;

		if (bufferHash){
			hm4::disk::FileBuilder::build(output_file,
							buffersWrite,
							std::begin(f()), std::end(f()),
							TOMBSTONE_OPTION, aligned,
							f().size(), bufferHash
			);
		}else{
			hm4::disk::FileBuilder::build(output_file,
							buffersWrite,
							std::begin(f()), std::end(f()),
							TOMBSTONE_OPTION, aligned
			);
		}
	}

} // anonymous namespace



struct MergeListFactory_2{
	MergeListFactory_2(std::string_view filename1, std::string_view filename2, DiskList::VMAllocator &allocator, DiskList::OpenMode const mode){
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
	MyDualList	table_{ table2_, table1_ };
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



constexpr size_t MIN_ARENA_SIZE	= 8;
constexpr size_t MB		= 1024 * 1024;

#include "disk/filebuilder.misc.h"
// defines g_fbwb;

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	size_t const bufferHashSize  = opt.hash_arena < MIN_ARENA_SIZE ? 0 : opt.hash_arena * MB;

	MyBuffer::MMapMemoryResource bufferHash{ bufferHashSize };

	auto buffersWrite = g_fbwb();

	return compact(opt, buffersWrite, bufferHash);
}

namespace{

	void printError(const char *msg){
		fmt::print("{}\n", msg);
		exit(1);
	}

	void printUsage(const char *cmd){
		fmt::print(
			"db_compact version {version}\n"
			"\n"
			"Build:\n"
			"\tDate       : {date} {time}\n"
			"\n"
			"Usage:\n"
			"\t{cmd} [configuration file] - perform smart merge\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("date",	__DATE__		),
			fmt::arg("time",	__TIME__		),
			fmt::arg("cmd",		cmd			)
		);

		fmt::print("INI File Usage:\n");

		MyOptions::print();

		fmt::print("\n");

		exit(10);
	}

	MyOptions prepareOptions(int argc, char **argv){
		MyOptions opt;

		switch(argc){
		case 1 + 1:
			if (! readINIFile(argv[1], opt))
				printError("Can not open config file...");

			break;

		default:
			printUsage(argv[0]);
		}

		return opt;
	}

	std::string getNewFilename(std::string_view db_path){
		constexpr std::string_view DIR_WILDCARD = "*";

		IDGenerator::to_string_buffer_t buffer;

		IDGenerator idGenerator;

		return StringReplace::replaceByCopy(db_path, DIR_WILDCARD, idGenerator(buffer));
	}



	auto getSize(std::string_view filename){
		hm4::disk::DiskList list;
		list.open(filename, DiskList::NoVMAllocator{}, DEFAULT_MODE);
		return list.size();
	}



	void renameFiles(MySpan<const std::string> const files, std::string_view from, std::string_view to){
		std::string buffer;

		for(auto const &file : files){
			MyGlob gl;
			gl.open( concatenateBuffer(buffer, file, "*") );

			fmt::print("Moving {}\n", file);

			for(std::string_view const file1 : gl){
				std::string const file2 = StringReplace::replaceByCopy(file1, from, to);

				fmt::print(" - {:<60} -> {}\n", file1, file2);

				rename(file1.data(), file2.data());
			}
		}

	}



	auto prepareSmartMergeFileList(MyOptions const &opt){
		using DBFilePair = std::pair<hm4::disk::DiskList::size_type, std::string_view>;

		std::vector<DBFilePair> files;

		struct Out{
			std::vector<std::string> zero;
			std::vector<std::string> path;
		};

		Out out;

		MyGlob gl;
		gl.open(opt.db_path);

		for(auto &filename : gl){
			auto const size = getSize(filename);

			if (size == 0){
			//	fmt::print("Zero len table: {}\n", file);

				out.zero.push_back(filename);

				continue;
			}

			files.emplace_back(size, filename);
		}

		std::sort(std::begin(files), std::end(files));



		auto &path = out.path;

		if constexpr(1){
			fmt::print("Sorted file list to consider:\n");

			for(auto &[size, filename] : files)
				fmt::print(" - {:<62} {:>18}\n", filename, size);

			fmt::print("\n");
		}



		size_t total = 0;

		for(auto &[size, filename] : files){
			// add first file...
			if (path.size() == 0){
				total = size;
				path.emplace_back(filename);
				continue;
			}

			// add small file...
			if (opt.compaction_min_records && size <= opt.compaction_min_records){
				total += size;
				path.emplace_back(filename);
				continue;
			}

			// add next file...
			if (auto const total_m = static_cast<size_t>(
					static_cast<double>(total) *
					( path.size() == 1 ? 1.25 : 1.00 )
				);
				size < opt.compaction_max_records	&&
				size < total_m
					){

				total += size;
				path.emplace_back(filename);

				continue;
			}

			if (path.size() > 1)
				break;

			path.clear();
			total = size;
			path.emplace_back(filename);
		}

		if (opt.compaction_max_tables > 1 && path.size() > opt.compaction_max_tables)
			path.resize(opt.compaction_max_tables);



		if constexpr(1){
			fmt::print("File list to compact\n");
			fmt::print("Total records ~ {}\n", total);

			for(auto &filename : path)
				fmt::print(" - {}\n", filename);

			fmt::print("\n");
		}



		return out;
	}



	int compact(MyOptions const &opt, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferHash){

		auto merge = [&opt, &buffersWrite, &bufferHash](std::string_view what, auto &&factory){
			auto const output_file = getNewFilename(opt.db_path);

			fmt::print("Merging {} tables into {}\n", what, output_file);

			mergeFromFactory(factory, output_file, buffersWrite, bufferHash);
		};

		DiskList::VMAllocator allocator{ g_slabBuffer };

		while(true){
			auto const out = prepareSmartMergeFileList(opt);

			// rename zero sized tables.
			// those suppose to be broken files.
			// node if a table is in process of creating,
			// it wont be recognized as zero sized.
			renameFiles(out.zero, opt.rename_from, opt.rename_to);

			auto &path = out.path;

			switch(path.size()){
			case 0:
			case 1:
				fmt::print("No need to compact...\n");

				return 0;

			case 2:
				merge( "two",
					MergeListFactory_2{ path[0], path[1],                 allocator, DEFAULT_MODE }
				);

				break;

			default:
				merge( "multiple",
					MergeListFactory_N{ std::begin(path), std::end(path), allocator, DEFAULT_MODE }
				);

				break;
			}

			renameFiles(path, opt.rename_from, opt.rename_to);

			fmt::print("\n\n\n");
			fmt::print("*** Loop processing ***\n");
			fmt::print("\n\n\n");
		}

		return 0;
	}

} // anonymous namespace

