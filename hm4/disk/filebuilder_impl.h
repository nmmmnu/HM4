#include "filemetablob.h"

#include <fstream>

#include "endian.h"
#include <limits>

namespace hm4{
namespace disk{


template <class ITERATOR>
bool FileBuilder::build(const StringRef &filename,
				const ITERATOR &begin, const ITERATOR &end,
				bool const keepTombstones, bool const aligned){

	std::ofstream fileMeta(filenameMeta(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileIndx(filenameIndx(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileData(filenameData(filename),	std::ios::out | std::ios::binary);

	return writeToFile__(begin, end, fileMeta, fileIndx, fileData, keepTombstones, aligned);
}


template <class ITERATOR>
bool FileBuilder::writeToFile__(const ITERATOR &begin, const ITERATOR &end,
				std::ofstream &file_meta,
				std::ofstream &file_indx,
				std::ofstream &file_data,
				bool const keepTombstones,
				bool const aligned){

	constexpr uint64_t MAX = std::numeric_limits<uint64_t>::min();

	uint64_t index		= 0;

	uint64_t count		= 0;
	uint64_t tombstones	= 0;
	uint64_t createdMin	= 0;
	uint64_t createdMax	= MAX;

	for(auto it = begin; it != end; ++it){
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
		uint64_t const be_index = htobe64( file_data.tellp() );
		file_indx.write( (const char *) & be_index, sizeof(uint64_t));

		// write the data
		pair.fwrite(file_data);

		index += pair.bytes();

		if (aligned){
			size_t const gap = pair.fwriteAlignGap(file_data);
			index += gap;
		}

		++count;
	}

	if (createdMax == MAX)
		createdMax = 0;

	// now we miraculasly have the datacount.

	uint16_t const option_aligned = aligned ? FileMetaBlob::OPTIONS_ALIGNED : FileMetaBlob::OPTIONS_NONE;

	// write table header
	uint16_t const options =
				FileMetaBlob::OPTIONS_NONE	|
				FileMetaBlob::OPTIONS_SORTED	|
				option_aligned
	;

	using hm4::disk::FileMetaBlob;

	const FileMetaBlob blob = FileMetaBlob::create( options, count, tombstones, createdMin, createdMax );

	file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));

	return true;
}


} // namespace
} // namespace hm4

