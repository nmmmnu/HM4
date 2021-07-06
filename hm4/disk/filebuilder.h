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

	enum class TombstoneOptions : bool{
		NONE,
		KEEP
	};

	class FileDataBuilder{
	public:
		FileDataBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions) :
							file_data(filenameData(filename), config::MODE),
							fileWriteOptions(fileWriteOptions){}

		void operator()(Pair const &pair);

		void flush(){
			file_data.flush();
		}

		void close(){
			file_data.close();
		}

	private:
		std::ofstream		file_data;

		Pair::WriteOptions	fileWriteOptions;
	};



	class FileIndxBuilder{
	public:
		FileIndxBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions) :
							file_indx(filenameIndx(filename), config::MODE),
							fileWriteOptions(fileWriteOptions){}

		void operator()(Pair const &pair);

	private:
		std::ofstream	file_indx;

		Pair::WriteOptions	fileWriteOptions;

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
		FileMetaBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions) :
							file_meta(filenameMeta(filename), config::MODE),
							fileWriteOptions(fileWriteOptions){}

		~FileMetaBuilder();

		void operator()(Pair const &pair);

	private:
		std::ofstream	file_meta;

		Pair::WriteOptions	fileWriteOptions;

		uint64_t	minCreated	= std::numeric_limits<uint64_t>::max();
		uint64_t	maxCreated	= std::numeric_limits<uint64_t>::min();

		uint64_t	count		= 0;
		uint64_t	tombstones	= 0;
	};



	class FileBuilder{
	public:
		using value_type = Pair const;

		FileBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions):
					meta(filename, fileWriteOptions	),
					indx(filename, fileWriteOptions	),
					line(filename			),
					data(filename, fileWriteOptions	){}

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
					TombstoneOptions const tombstoneOptions, Pair::WriteOptions const fileWriteOptions){

		FileBuilder builder(filename, fileWriteOptions);

		if (tombstoneOptions == TombstoneOptions::KEEP){

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

