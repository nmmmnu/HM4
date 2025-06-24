#ifndef DISK_FILE_BUILDER_CONF_H_
#define DISK_FILE_BUILDER_CONF_H_

#include "mybufferview.h"

namespace hm4::disk::FileBuilder{

	struct FileBuilderWriteBuffers{
		MyBuffer::ByteBufferView data;
		MyBuffer::ByteBufferView indx;
		MyBuffer::ByteBufferView line;
		MyBuffer::ByteBufferView hash;
	};

} // namespace hm4::disk::FileBuilder

#endif

