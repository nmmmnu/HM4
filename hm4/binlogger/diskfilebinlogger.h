#ifndef DISK_FILE_BINLOG_H_
#define DISK_FILE_BINLOG_H_

#include "disk/filebuilder.h"
#include "myfs.h"
//#include "staticbuffer.h"
#include "mybufferview.h"

namespace hm4{
namespace binlogger{


class DiskFileBinLogger{
private:
	using FileDataBuilder = disk::FileBuilder::FileDataBuilder;

public:
	enum class SyncOptions : bool{
		NONE	= false,
		FSYNC	= true
	};

	template<typename UString>
	DiskFileBinLogger(UString &&filename, SyncOptions const syncOprions, Pair::WriteOptions const writeOptions):
				filename_	(std::forward<UString>(filename)	),
				syncOprions_	(syncOprions				),
				writeOptions_	(writeOptions				){}

	template<typename UString>
	DiskFileBinLogger(UString &&filename, Pair::WriteOptions const writeOptions):
				DiskFileBinLogger(
					std::forward<UString>(filename),
					SyncOptions::NONE,
					writeOptions
				){}

public:
	void operator()(Pair const &pair){
		dataBuilder_(pair);

		if (syncOprions_ == SyncOptions::FSYNC)
			flush();
	}

	void operator()(Pair const *pair){
		return operator()(*pair);
	}

	bool clear(){
		dataBuilder_.close();
		// file is sync to disk.
		FileDataBuilder temp{ filename_, MyBuffer::ByteBufferView{}, writeOptions_ };
		dataBuilder_ = std::move(temp);
		return true;
	}

	bool unlinkFile(){
		dataBuilder_.close();
		// file is sync to disk.
		fileUnlink(disk::filenameData(filename_));
		return true;
	}

	void flush(){
		dataBuilder_.flush();
	}

private:
	std::string		filename_;
	SyncOptions		syncOprions_;
	Pair::WriteOptions	writeOptions_;
	FileDataBuilder		dataBuilder_{ filename_, MyBuffer::ByteBufferView{}, writeOptions_ };
};


} // namespace flusher
} // namespace

#endif

