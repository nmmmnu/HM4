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

#include "mytime.h"

using hm4::disk::DiskList;

constexpr auto DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto DEFAULT_MODE	= DiskList::OpenMode::FORWARD;

using IDGenerator = idgenerator::IDGeneratorDate;

using hm4::disk::FileBuilder::TombstoneOptions;

constexpr TombstoneOptions TOMBSTONE_OPTION = TombstoneOptions::REMOVE;



namespace{

	MyOptions prepareOptions(int argc, char **argv);

	int compact(MyOptions const &opt);

	template <class Factory>
	void mergeFromFactory(Factory const &f, std::string_view output_file){
		hm4::disk::FileBuilder::build(output_file, std::begin(f()), std::end(f()), TOMBSTONE_OPTION, hm4::Pair::WriteOptions::ALIGNED);
	}

} // anonymous namespace



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
			"db_compact_tombstones version {version}\n"
			"\tTHIS FILE IS NOT PRODUCTION READY!!!\n"
			"\n"
			"Build:\n"
			"\tDate       : {date} {time}\n"
			"\n"
			"Usage:\n"
			"\t{cmd} [configuration file] - perform smart remove of tombstones\n"
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

#if 0
	std::string getNewFilename(std::string_view db_path){
		constexpr std::string_view DIR_WILDCARD = "*";

		IDGenerator::to_string_buffer_t buffer;

		IDGenerator idGenerator;

		return StringReplace::replaceByCopy(db_path, DIR_WILDCARD, idGenerator(buffer));
	}

	void renameFiles(MySpan<const std::string> const files, std::string_view from, std::string_view to){
		std::string buffer;

		for(auto const &file : files){
			MyGlob gl;
			gl.open( concatenateBuffer(buffer, file, "*") );

			fmt::print("Moving {}\n", file);

			for(std::string_view const file1 : gl){
				std::string const file2 = StringReplace::replaceByCopy(file1, from, to);

				fmt::print(" -{:<60} -> {}\n", file1, file2);

				rename(file1.data(), file2.data());
			}
		}

	}
#endif



	auto getInfo(std::string_view filename){
		hm4::disk::DiskList list;
		list.open(filename, DEFAULT_ADVICE, DEFAULT_MODE);

		struct ListInfo{
			uint64_t time_min;
			uint64_t time_max;
		};

		return std::make_pair(
			list.createdMin(),
			list.createdMax()
		);
	}



	auto prepareSmartMergeFileList(MyOptions const &opt){
		const char *mask = " - {:<25} | {:<62} | {:>8} | {:>8} | {:>8}\n";

		constexpr std::string_view filename_none = "n/a";

		uint64_t time = 0;

		std::string_view filename_leader = filename_none;

		mytime::to_string_buffer_t buffer[3];

		auto x = [](auto date, auto &buffer){
			constexpr auto time_format = mytime::TIME_FORMAT_STANDARD;

			return date    ? mytime::toString(time_format, buffer) : "n/a";
		};

		MyGlob gl;
		gl.open(opt.db_path);

		for(auto &filename : gl){
			auto const [time_min, time_max] = getInfo(filename);

			if (time < time_min){
				// new segment
				time = time_max;

				filename_leader = filename;

				fmt::print(mask, "New segment / lead",		filename, x(time_min, buffer[0]), x(time_max, buffer[1]), x(time, buffer[2])	);
			}else if (time > time_max){
				// past segment skip...
				fmt::print(mask, "Past segment / skip",		filename, x(time_min, buffer[0]), x(time_max, buffer[1]), x(time, buffer[2])	);
			}else{
				// overlapping segment
				filename_leader = filename_none;

				fmt::print(mask, "Overlapping segment / reset",	filename, x(time_min, buffer[0]), x(time_max, buffer[1]), x(time, buffer[2])	);
			}
		}

		return std::string{ filename_leader };
	}



	int compact(MyOptions const &opt){
		auto const filename = prepareSmartMergeFileList(opt);

		fmt::print("You can consider to compact following file:\n{}\n", filename);

		return 0;
	}

} // anonymous namespace

