#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include <fstream>
#include <algorithm>
#include <limits>

#include "disk/filenames.h"

#include "hpair.h"
#include "stringhash.h"

namespace hm4{
namespace disk{
namespace FileBuilder{

	namespace config{
		constexpr std::ios::openmode MODE = std::ios::out | std::ios::binary | std::ios::trunc;
	}



	class FileDataBuilder{
	public:
		FileDataBuilder(std::string_view const filename, bool const aligned) :
							file_data(filenameData(filename), config::MODE),
							aligned(aligned){}

		void operator()(Pair const &pair);

		void flush(){
			file_data.flush();
		}

		void close(){
			file_data.close();
		}

	private:
		std::ofstream	file_data;

		bool		aligned;
	};



	class FileIndxBuilder{
	public:
		FileIndxBuilder(std::string_view const filename, bool const aligned) :
							file_indx(filenameIndx(filename), config::MODE),
							aligned(aligned){}

		void operator()(Pair const &pair);

	private:
		std::ofstream	file_indx;

		bool		aligned;

		uint64_t	index		= 0;
	};



	class FileLineBuilder{
	public:
		FileLineBuilder(std::string_view const filename) :
							file_line(filenameLine(filename), config::MODE){}

		void operator()(Pair const &pair);

	private:
		std::ofstream	file_line;

		uint64_t	pos		= 0;

		HPair::HKey	hkey_		= 0;
	};



	class FileMetaBuilder{
	public:
		FileMetaBuilder(std::string_view const filename, bool const aligned) :
							file_meta(filenameMeta(filename), config::MODE),
							aligned(aligned){}

		~FileMetaBuilder();

		void operator()(Pair const &pair);

	private:
		std::ofstream	file_meta;

		bool		aligned;

		uint64_t	minCreated	= std::numeric_limits<uint64_t>::max();
		uint64_t	maxCreated	= std::numeric_limits<uint64_t>::min();

		uint64_t	count		= 0;
		uint64_t	tombstones	= 0;
	};



	class FileBuilder{
	public:
		using value_type = Pair const;

		FileBuilder(std::string_view const filename, bool const aligned):
					meta(filename, aligned),
					indx(filename, aligned),
					line(filename         ),
					data(filename, aligned){}

		void operator()(Pair const &pair){
			push_back(pair);
		}

		void push_back(Pair const &pair){
			meta(pair);
			indx(pair);
			line(pair);
			data(pair);
		}

	private:
		FileMetaBuilder meta;
		FileIndxBuilder indx;
		FileLineBuilder line;
		FileDataBuilder data;
	};



	// ==============================

	template <class IT>
	bool build(std::string_view const filename, IT first, IT last,
					bool const keepTombstones, bool const aligned){

		FileBuilder builder(filename, aligned);

		if (keepTombstones){

			// invalid pairs must be kept as tombstones
			std::copy(first, last, std::back_inserter(builder));

		}else{

			std::copy_if(first, last, std::back_inserter(builder), [](Pair const &pair){
				if (!pair.isValid())
					return false;

				if (pair.isTombstone())
					return false;

				return true;
			});

		}

		return true;
	}

} // namespace
} // namespace
} // namespace hm4


#endif

