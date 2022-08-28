#include "filemetablob.h"

#include "myendian.h"
#include "mytime.h"

#include <cstring>	// strcpy


namespace hm4{
namespace disk{


FileMetaBlob FileMetaBlob::create(uint16_t const options, uint64_t const count, uint64_t const tombstones, uint64_t const createdMin, uint64_t const createdMax){
	FileMetaBlob blob;

	strcpy(blob.logo, FileMetaBlob::LOGO);

	blob.version	= htobe<uint16_t>(FileMetaBlob::VERSION	);
	blob.options	= htobe<uint16_t>(options		);
	blob.created	= htobe<uint32_t>(mytime::now32()	);

	blob.size	= htobe<uint64_t>(count			);
	blob.tombstones	= htobe<uint64_t>(tombstones		);
	blob.createdMin	= htobe<uint64_t>(createdMin		);
	blob.createdMax	= htobe<uint64_t>(createdMax		);

	return blob;
}


} // namespace
} // namespace hm4

