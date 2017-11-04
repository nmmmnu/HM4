#include "filemetablob.h"

#include <fstream>

#include "endian.h"
#include <limits>

#include "myalign.h"
#include "pair.h"

#include "mynarrow.h"

namespace hm4{
namespace disk{


template <class ITERATOR>
bool FileBuilder::build(const StringRef &filename,
				const ITERATOR &begin, const ITERATOR &end,
				bool const keepTombstones, bool const aligned){

	std::ofstream fileMeta(filenameMeta(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileIndx(filenameIndx(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileLine(filenameLine(filename),	std::ios::out | std::ios::binary);
	std::ofstream fileData(filenameData(filename),	std::ios::out | std::ios::binary);

	return writeToFile__(begin, end, fileMeta, fileIndx, fileLine, fileData, keepTombstones, aligned);
}


template <class ITERATOR>
bool FileBuilder::writeToFile__(const ITERATOR &begin, const ITERATOR &end,
				std::ofstream &file_meta,
				std::ofstream &file_indx,
				std::ofstream &file_line,
				std::ofstream &file_data,
				bool const keepTombstones,
				bool const aligned){

	uint64_t index		= 0;

	uint64_t count		= 0;
	uint64_t tombstones	= 0;

	// set min / max
	uint64_t createdMin	= std::numeric_limits<uint64_t>::max();
	uint64_t createdMax	= std::numeric_limits<uint64_t>::min();

	for(auto it = begin; it != end; ++it){
		const Pair &pair = *it;

		if (!pair.isValid())
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

		/* write the index */
		{
			// this is way slower.
			// uint64_t const be_index = htobe64( file_data.tellp() );

			uint64_t const be_index = htobe64(index);

			file_indx.write( (const char *) & be_index, sizeof(uint64_t));
		}

		/* white cache line */
		{
			const auto &key = pair.getKey();

			constexpr auto HLINE_SIZE = PairConf::HLINE_SIZE;

			if (key.size() >= HLINE_SIZE){
				file_line.write(key.data(), HLINE_SIZE);

			}else{
				file_line.write(key.data(), narrow<std::streamsize>(key.size()));

				constexpr MyAlign<HLINE_SIZE> alc;

				alc.fwriteGap(file_line, key.size());
			}
		}

		/* white the data */
		{
			pair.fwrite(file_data);

			size_t bytes = pair.bytes();


			if (aligned){
				constexpr MyAlign<PairConf::ALIGN> alc;

				bytes += alc.fwriteGap(file_data, pair.bytes());
			}

			index += bytes;
		}

		++count;
	}

	// fix min / max -> 0

	if (createdMin == std::numeric_limits<uint64_t>::max())
		createdMin = 0;

	if (createdMax == std::numeric_limits<uint64_t>::min())
		createdMin = 0;

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

