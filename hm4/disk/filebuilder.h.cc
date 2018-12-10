#include "filemetablob.h"

#include <fstream>

#include <limits>

#include "myalign.h"
#include "pair.h"

#include "myendian.h"
#include "mynarrow.h"

namespace hm4{
namespace disk{

namespace FileBuilder{

	namespace filebuilder_impl_{

		inline void writeU64(std::ofstream &file, uint64_t const data){
			uint64_t const be_data = htobe(data);

			file.write( (const char *) & be_data, sizeof(uint64_t));
		}

		template<size_t HLINE_SIZE>
		void writeStr(std::ofstream &file, const SmallString<HLINE_SIZE> &key){
			file.write(key.data(), narrow<std::streamsize>(key.size()));

			constexpr MyAlign<HLINE_SIZE> alc;

			alc.fwriteGap(file, key.size());
		}


		// ==============================


		class CacheLineBuilder{
		private:
			constexpr static auto HLINE_SIZE = PairConf::HLINE_SIZE;

		public:
			CacheLineBuilder(std::ofstream &file) : file_(file){}

			~CacheLineBuilder(){
				store_();
			}

			void operator()(StringRef const &current, uint64_t const pos){
				if (key_.equals(current))
					return;

				store_();

				// introduce new key
				key_ = current;
				pos_ = pos;
			}

		private:
			void store_(){
				if (key_.empty())
					return;

				writeStr(file_, key_);
				writeU64(file_, pos_);
			}

		private:
			SmallString<HLINE_SIZE>	key_;
			uint64_t		pos_	= 0;
			std::ofstream		&file_;
		};


		// ==============================


		template <class IT>
		bool writeToFile(IT first, IT last,
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

			CacheLineBuilder cacheLine(file_line);

			for(; first != last; ++first){
				const Pair &pair = *first;

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

				// ==============================

				// write the index
				writeU64(file_indx, index);

				// white cache line
				cacheLine(pair.getKey(), count);

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

	} // namespace filebuilder_impl_


	template <class IT>
	bool build(StringRef const &filename,
					IT first, const IT last,
					bool const keepTombstones, bool const aligned){

		constexpr auto mode = std::ios::out | std::ios::binary;

		std::ofstream fileMeta(filenameMeta(filename),	mode);
		std::ofstream fileIndx(filenameIndx(filename),	mode);
		std::ofstream fileLine(filenameLine(filename),	mode);
		std::ofstream fileData(filenameData(filename),	mode);

		using namespace filebuilder_impl_;

		return writeToFile(std::move(first), std::move(last), fileMeta, fileIndx, fileLine, fileData, keepTombstones, aligned);
	}

} // namespace

} // namespace
} // namespace hm4

