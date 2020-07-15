#ifndef DISK_FILE_BINLOG_H_
#define DISK_FILE_BINLOG_H_

#include "disk/filebuilder.h"
#include "myfs.h"


namespace hm4{
namespace binlogger{


class DiskFileBinLogger{
private:
	using FileDataBuilder = disk::FileBuilder::FileDataBuilder;

public:
	template<typename UString>
	DiskFileBinLogger(UString &&filename, bool const fsync, bool const aligned):
				filename_	(std::forward<UString>(filename)	),
				fsync_		(fsync					),
				aligned_	(aligned				){}

	template<typename UString>
	DiskFileBinLogger(UString &&filename, bool const aligned):
				DiskFileBinLogger(
					std::forward<UString>(filename),
					/* fsync */ false,
					aligned
				){}

public:
	void operator()(Pair const &pair){
		dataBuilder_(pair);

		if (fsync_)
			dataBuilder_.flush();
	}

	bool clear(){
		dataBuilder_.close();
		// file is sync to disk.
		dataBuilder_ = { filename_, aligned_ };
		return true;
	}

	bool unlinkFile(){
		dataBuilder_.close();
		// file is sync to disk.

		fileUnlink(disk::filenameData(filename_));

		return true;
	}

private:
	std::string	filename_;
	bool		fsync_;
	bool		aligned_;
	FileDataBuilder	dataBuilder_{ filename_, aligned_ };
};


} // namespace flusher
} // namespace

#endif

