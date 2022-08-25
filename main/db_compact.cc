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



namespace{

	MyOptions prepareOptions(int argc, char **argv);

	int compact(MyOptions const &opt);

	template <class Factory>
	void mergeFromFactory(Factory const &f, std::string_view output_file){
		hm4::disk::FileBuilder::build(output_file, std::begin(f()), std::end(f()), TOMBSTONE_OPTION, hm4::Pair::WriteOptions::ALIGNED);
	}

} // anonymous namespace



struct MergeListFactory_2{
	MergeListFactory_2(std::string_view filename1, std::string_view filename2, const MMAPFile::Advice advice, DiskList::OpenMode const mode){
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



int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	return compact(opt);
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
			"Usage:\n"
			"\t{cmd} [configuration file] - perform smart merge\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
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
		list.open(filename, DEFAULT_ADVICE, DEFAULT_MODE);
		return list.size();
	}



	std::vector<std::string> prepareSmartMergeFileList(MyOptions const &opt){
		using DBFilePair = std::pair<hm4::disk::DiskList::size_type, std::string_view>;

		std::vector<DBFilePair> files;

		MyGlob gl;
		gl.open(opt.db_path);


		for(auto &file : gl){
			auto const size = getSize(file);

			if (size == 0)
				continue;

			files.emplace_back(size, file);
		}

		std::sort(std::begin(files), std::end(files));



		if constexpr(1){
			fmt::print("Sorted file list to consider:\n");

			for(auto &[size, filename] : files)
				fmt::print("\t- {:<62} {:>8}\n", filename, size);

			fmt::print("\n");
		}



		std::vector<std::string> out;

		size_t total = 0;

		for(auto &[size, filename] : files){
			if (out.size() == 0){
				total += size;
				out.emplace_back(filename);
				continue;
			}

			if (opt.compaction_min_records && size <= opt.compaction_min_records){
				total += size;
				out.emplace_back(filename);
				continue;
			}

			if (auto const total_m = static_cast<size_t>(
					static_cast<double>(total) *
					( out.size() == 1 ? 1.25 : 1.00 )
				);
				size	< opt.compaction_max_records					&&
				total_m	< opt.compaction_max_records * opt.compaction_max_tables	&&
				size < total_m
					){

				total += size;
				out.emplace_back(filename);
				continue;
			}

			if (out.size() > 1)
				break;

			out.clear();
		}

		if (opt.compaction_max_tables > 1 && out.size() > opt.compaction_max_tables)
			out.resize(opt.compaction_max_tables);



		if constexpr(1){
			fmt::print("File list to compact (Cumulative size {}):\n", total);

			for(auto &filename : out)
				fmt::print("\t- {}\n", filename);

			fmt::print("\n");
		}



		return out;
	}



	void renameFiles(MySpan<std::string> const files, std::string_view from, std::string_view to){
		std::string buffer;

		for(auto const &file : files){
			MyGlob gl;
			gl.open( concatenateBuffer(buffer, file, "*") );

			for(std::string_view const file1 : gl){
				std::string const file2 = StringReplace::replaceByCopy(file1, from, to);

			//	fmt::print("mv {:<60} -> {}\n", file1, file2);

				rename(file1.data(), file2.data());
			}
		}

	}



	int compact(MyOptions const &opt){
		while(true){
			auto const path = prepareSmartMergeFileList(opt);

			switch(path.size()){
			case 0:
			case 1:
				{
					fmt::print(	"No need to compact...\n");

					return 0;
				}

			case 2:
				{
					auto const output_file = getNewFilename(opt.db_path);

					fmt::print(	"Merging two tables into {}\n", output_file);

					MergeListFactory_2 factory{ path[0], path[1], DEFAULT_ADVICE, DEFAULT_MODE };

					mergeFromFactory(factory, output_file);

					break;
				}

			default:
				{
					auto const output_file = getNewFilename(opt.db_path);

					fmt::print(	"Merging multiple tables into {}\n", output_file);

					MergeListFactory_N factory{ std::begin(path), std::end(path), DEFAULT_ADVICE, DEFAULT_MODE };

					mergeFromFactory(factory, output_file);

					break;
				}
			}

			renameFiles(path, opt.rename_from, opt.rename_to);

			fmt::print("\n\n\n");
			fmt::print("*** Loop processing ***\n");
			fmt::print("\n\n\n");
		}

		return 0;
	}

} // anonymous namespace

