#ifndef DISK_FILE_BUILDER_MISC_H_
#define DISK_FILE_BUILDER_MISC_H_

#include "disk/filebuilder.conf.h"

namespace hm4::disk::FileBuilder{

	class FileBuilderWriteStaticBuffers{
		constexpr static size_t KB = 1024;
		constexpr static size_t DataSize = 64;
		constexpr static size_t PageSize =  4;

		MyBuffer::StaticMemoryResource<DataSize * KB> data;
		MyBuffer::StaticMemoryResource<PageSize * KB> indx;
		MyBuffer::StaticMemoryResource<PageSize * KB> line;
		MyBuffer::StaticMemoryResource<PageSize * KB> hash;

	public:
		auto operator()(){
			return FileBuilderWriteBuffers{
				data,
				indx,
				line,
				hash
			};
		}
	};

} // namespace hm4::disk::FileBuilder



hm4::disk::FileBuilder::FileBuilderWriteStaticBuffers g_fbwb;



#endif

