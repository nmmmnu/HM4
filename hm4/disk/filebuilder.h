#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include <algorithm>
#include <limits>

#include "disk/filenames.h"

#include "hpair.h"
#include "stringhash.h"
#include "filewriter.h"

namespace hm4{
namespace disk{
namespace FileBuilder{
	enum class TombstoneOptions : bool{
		REMOVE,
		KEEP
	};

	class FileDataBuilder{
	public:
		FileDataBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions) :
							file_data(filenameData(filename)),
							fileWriteOptions(fileWriteOptions){}

		void operator()(Pair const &pair);

		void flush(){
			file_data.flush();
		}

		void close(){
			file_data.close();
		}

	private:
		FileWriter		file_data;

		Pair::WriteOptions	fileWriteOptions;
	};



	class FileIndxBuilder{
	public:
		FileIndxBuilder(std::string_view const filename, Pair::WriteOptions const fileWriteOptions) :
							file_indx(filenameIndx(filename)),
							fileWriteOptions(fileWriteOptions){}

		void operator()(Pair const &pair);

	private:
		FileWriter		file_indx;

		Pair::WriteOptions	fileWriteOptions;

		uint64_t	index		= 0;
	};



	class FileLineBuilder{
	public:
		FileLineBuilder(std::string_view const filename) :
							file_line(filenameLine(filename)){}

		void operator()(Pair const &pair);

	private:
		FileWriter	file_line;

		uint64_t	pos		= 0;

		HPair::HKey	hkey_		= 0;
	};



	class FileMetaBuilder{
	public:
		template<typename UString>
		FileMetaBuilder(UString &&filename, Pair::WriteOptions const fileWriteOptions) :
							fileWriteOptions(fileWriteOptions),
							filename(std::forward<UString>(filename)){}

		~FileMetaBuilder();

		void operator()(Pair const &pair);

	private:
		Pair::WriteOptions	fileWriteOptions;

		std::string		filename;

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
				return pair.isOK();
			});

		}

		return true;
	}



} // namespace
} // namespace
} // namespace hm4


#endif

