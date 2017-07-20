#include "filemetablob.h"

#include "endian.h"
#include "mytime.h"


namespace hm4{
namespace disk{


FileMetaBlob FileMetaBlob::create(uint16_t const options, uint64_t const count, uint64_t const tombstones, uint64_t const createdMin, uint64_t const createdMax){
	FileMetaBlob blob;

	strcpy(blob.logo, FileMetaBlob::LOGO);

	blob.version	= htobe16(FileMetaBlob::VERSION	);
	blob.options	= htobe16(options		);
	blob.created	= htobe32(MyTime::now32()	);

	blob.size	= htobe64(count			);
	blob.tombstones	= htobe64(tombstones		);
	blob.createdMin	= htobe64(createdMin		);
	blob.createdMax	= htobe64(createdMax		);

	return blob;
}


} // namespace
} // namespace hm4

