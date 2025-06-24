#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include <algorithm>
#include <limits>

#include "disk/filenames.h"
#include "disk/filebuilder.conf.h"

#include "hpair.h"
#include "stringhash.h"
#include "filewriter.h"

#include "hashindexbuilder.h"

namespace hm4{
namespace disk{
namespace FileBuilder{
	enum class TombstoneOptions : bool{
		REMOVE,
		KEEP
	};

	struct FileDataBuilder{
		FileDataBuilder(std::string_view const filename, MyBuffer::ByteBufferView buffer, Pair::WriteOptions const fileWriteOptions) :
							file_data(filenameData(filename), buffer),
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



	struct FileIndxBuilder{
		FileIndxBuilder(std::string_view const filename, MyBuffer::ByteBufferView buffer, Pair::WriteOptions const fileWriteOptions) :
							file_indx(filenameIndx(filename), buffer),
							fileWriteOptions(fileWriteOptions){}

		void operator()(Pair const &pair);

	private:
		FileWriter		file_indx;

		Pair::WriteOptions	fileWriteOptions;

		uint64_t	index		= 0;
	};



	struct FileLineBuilder{
		FileLineBuilder(std::string_view const filename, MyBuffer::ByteBufferView buffer) :
							file_line(filenameLine(filename), buffer){}

		void operator()(Pair const &pair);

	private:
		FileWriter	file_line;

		uint64_t	pos		= 0;

		HPair::HKey	hkey_		= 0;
	};



	struct FileMetaBuilder{
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



	template<bool WithHash>
	struct FileBuilder{
		using value_type = Pair const;

		FileBuilder(std::string_view const filename, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, Pair::WriteOptions const fileWriteOptions):
					meta(filename,                    fileWriteOptions		),
					indx(filename, buffersWrite.indx, fileWriteOptions		),
					line(filename, buffersWrite.line				),
					data(filename, buffersWrite.data, fileWriteOptions		){}

		FileBuilder(std::string_view const filename, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, Pair::WriteOptions const fileWriteOptions, size_t listSize, MyBuffer::ByteBufferView bufferHash):
					meta(filename,                    fileWriteOptions		),
					indx(filename, buffersWrite.indx, fileWriteOptions		),
					line(filename, buffersWrite.line				),
					data(filename, buffersWrite.data, fileWriteOptions		),
					hazh(filename, buffersWrite.hash, listSize, bufferHash	){}

		void operator()(Pair const &pair){
			push_back(pair);
		}

		void push_back(Pair const &pair){
			meta(pair);
			indx(pair);
			line(pair);
			data(pair);

			if constexpr(!std::is_same_v<FileHashBuilder, std::nullptr_t>){
			hazh(pair);
			}
		}

	private:
		using FileHashBuilder = std::conditional_t<WithHash, hash::HashIndexBuilder, std::nullptr_t>;

	private:
		FileMetaBuilder meta;
		FileIndxBuilder indx;
		FileLineBuilder line;
		FileDataBuilder data;
		FileHashBuilder hazh;
	};



	// ==============================


	namespace FileBuilderImpl_{

		template <class IT, bool B>
		bool copy(FileBuilder<B> &&builder, IT first, IT last,
						TombstoneOptions const tombstoneOptions){

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

	} // namespace FileBuilderImpl_

	template <class IT>
	bool build(std::string_view const filename, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, IT first, IT last, TombstoneOptions const tombstoneOptions,
					Pair::WriteOptions const fileWriteOptions,
					size_t listSize, MyBuffer::ByteBufferView bufferHash){

		return FileBuilderImpl_::copy(
				FileBuilder<1>(filename, buffersWrite, fileWriteOptions, listSize, bufferHash),
				first, last, tombstoneOptions
		);
	}

	template <class IT>
	bool build(std::string_view const filename, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, IT first, IT last, TombstoneOptions const tombstoneOptions,
					Pair::WriteOptions const fileWriteOptions){

		return FileBuilderImpl_::copy(
				FileBuilder<0>(filename, buffersWrite, fileWriteOptions),
				first, last, tombstoneOptions
		);
	}

} // namespace
} // namespace
} // namespace hm4


#endif

