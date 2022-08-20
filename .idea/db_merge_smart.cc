#include "disk/filebuilder.h"

#include "version.h"

#include "myglob.h"
#include "stringreplace.h"
#include "idgenerator.h"
#include "disk/disklist.h"

#include <vector>

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

using hm4::disk::DiskList;



constexpr auto	DEFAULT_ADVICE	= MMAPFile::Advice::SEQUENTIAL;
constexpr auto	DEFAULT_MODE	= DiskList::OpenMode::FORWARD;



using IDGenerator = idgenerator::IDGeneratorDate;



namespace{

	int printUsage(const char *cmd){
		fmt::print(	"db_merge_smart version {0} \n"
				"\n"
				"Usage:\n"
				"\t{1} [lsm_path]\n"
				"\n"
				"\t\tPath names must be written with quotes:\n"
				"\t\t\tExample directory/file.'*'.db\n"
				"\t\t\tThe '*', will be replaced with ID's\n"
				"\n",
				hm4::version::str,
				cmd
		);

		return 10;
	}



	auto prepareSmartMerge(std::string_view mask, double const percent = 0.6666){
		struct DBFile{
			using size_type = hm4::disk::DiskList::size_type;

			std::string_view	filename;
			size_type		size;

			DBFile(std::string_view filename) :
						filename	(filename			),
						size		(getSize__(this->filename)	){}

			bool operator <(DBFile const &other) const{
				return size < other.size;
			}

		private:
			static size_type getSize__(std::string_view filename){
				hm4::disk::DiskList list;
				list.open(filename, DEFAULT_ADVICE, DEFAULT_MODE);
				return list.size();
			}
		};

		std::vector<DBFile> files;

		MyGlob gl;
		gl.open(mask);

		fmt::print(stderr, "File list to consider:\n");

		for(auto &file : gl){
			files.emplace_back(file);

			fmt::print(stderr, "\t- {:<62} {:>8}\n", files.back().filename, files.back().size);
		}

		std::sort(std::begin(files), std::end(files));

		std::vector<std::string> out;

		size_t total = 0;

		for(size_t i = 0; i < files.size(); ++i){
			auto &f = files[i];

			if (f.size == 0)
				continue;

			if (total == 0 || static_cast<double>(total) > static_cast<double>(f.size) * percent){
				total += f.size;
				out.emplace_back(f.filename);
			}else
				break;
		}

	//	if (out.size() == 1)
	//		out.clear();

		return out;
	}



	std::string getNewFilename(std::string_view path){
		constexpr std::string_view DIR_WILDCARD = "*";

		IDGenerator::to_string_buffer_t buffer;

		IDGenerator idGenerator;

		return StringReplace::replaceByCopy(path, DIR_WILDCARD, idGenerator(buffer));
	}

} // anonymous namespace



int main(int argc, char **argv){
	if (argc <= 1)
		return printUsage(argv[0]);

	std::string_view const path = argv[1];

	auto files = prepareSmartMerge(path);

	if (files.size() < 2){
		fmt::print(stderr, "No need to merge anything...\n");

		return 0;
	}

	fmt::print("$DB_MERGE - '{}' ", getNewFilename(path));
	for(auto const &file : files)
		fmt::print("'{}' ", file);
	fmt::print("\n\n");

	fmt::print("mv ");
	for(auto const &file : files)
		fmt::print("'{}'* ", file);
	fmt::print("\"$BACKUP_DIR\"\n\n");

	return 0;
}

