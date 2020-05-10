#ifndef DISK_FILE_BINLOG_H_
#define DISK_FILE_BINLOG_H_

#include "disk/filebuilder.h"

namespace hm4{
namespace binlogger{


class DiskFileBinLogger{
private:
	using FileDataBuilder = disk::FileBuilder::FileDataBuilder;

public:
	template<typename UString>
	DiskFileBinLogger(UString &&filename, bool const aligned):
				filename_	(std::forward<UString>(filename)	),
				aligned_	(aligned				){}

public:
	void operator()(Pair const &pair){
		dataBuilder_(pair);
	}

	bool clear(){
		dataBuilder_ = { filename_, aligned_ };
		return true;
	}

private:
	std::string	filename_;
	bool		aligned_;
	FileDataBuilder	dataBuilder_{ filename_, aligned_ };
};


} // namespace flusher
} // namespace

#endif

