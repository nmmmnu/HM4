#ifndef DISK_FILE_BINLOG_H_
#define DISK_FILE_BINLOG_H_

#include "disk/filebuilder.h"

namespace hm4{
namespace binlogger{


class DiskFileBinLogger{
public:
	DiskFileBinLogger(std::string_view const filename, bool const aligned):
				dataBuilder_(filename, aligned){}

public:
	void operator()(Pair const &pair){
		dataBuilder_(pair);
	}

	bool clear(){
		// implement reset somehow
		return true;
	}

private:
	disk::FileBuilder::FileDataBuilder dataBuilder_;
};


} // namespace flusher
} // namespace

#endif

