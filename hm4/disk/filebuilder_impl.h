#include "filemetablob.h"
#include "endian.h"

namespace hm4{
namespace disk{


template <class ITERATOR>
bool DiskFileBuilder::build(const StringRef &filename,
				const ITERATOR &begin, const ITERATOR &end,
				bool const keepTombstones){

	std::ofstream fileMeta(filenameMeta(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileIndx(filenameIndx(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileData(filenameData(filename),	std::ios::out | std::ios::binary);

	return writeToFile__(begin, end, fileMeta, fileIndx, fileData, keepTombstones);
}


template <class ITERATOR>
bool DiskFileBuilder::writeToFile__(const ITERATOR &begin, const ITERATOR &end,
				std::ofstream &file_meta,
				std::ofstream &file_indx,
				std::ofstream &file_data,
				bool const keepTombstones){

	size_t	index		= 0;

	uint64_t count		= 0;
	uint64_t tombstones	= 0;
	uint64_t createdMin	= begin->getCreated();
	uint64_t createdMax	= createdMin;

	for(ITERATOR it = begin; it != end; ++it){
		const auto &pair = *it;

		if (! pair )
			continue;

		if (pair.isTombstone()){
			if (keepTombstones == false)
					continue;

			++tombstones;
		}

		uint64_t const created = pair.getCreated();

		if (created < createdMin)
			createdMin = created;

		if (created > createdMax)
			createdMax = created;

		// write the index
		uint64_t const be_index = htobe64(index);
		file_indx.write( (const char *) & be_index, sizeof(uint64_t));

		// write the data
		pair.fwrite(file_data);

		index += pair.bytes();
		++count;
	}

	// now we miraculasly have the datacount.

	// write table header
	uint16_t const options =
				FileMetaBlob::OPTIONS_NONE	|
				FileMetaBlob::OPTIONS_SORTED
	;

	using hm4::disk::FileMetaBlob;

	const FileMetaBlob blob = FileMetaBlob::create( options, count, tombstones, createdMin, createdMax );

	file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));

	return true;
}


} // namespace
} // namespace hm4

