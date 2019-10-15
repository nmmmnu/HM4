#include "filebuilder.h"

#include "disk/filenames.h"

#include "filemetablob.h"

#include "myalign.h"
#include "myendian.h"

namespace hm4{
namespace disk{

namespace FileBuilder{
	namespace{
		void writeU64(std::ofstream &file, uint64_t const data){
			uint64_t const be_data = htobe(data);

			file.write( (const char *) & be_data, sizeof(uint64_t));
		}

		template<size_t HLINE_SIZE>
		void writeStr(std::ofstream &file, const SmallString<HLINE_SIZE> &key){
			file.write(key.data(), narrow<std::streamsize>(key.size()));

			my_align::fwriteGap(file, key.size(), HLINE_SIZE);
		}

		template<typename T>
		T fixMinMax(T const val, T const limit, T const fallback = 0){
			return val == limit ? fallback : val;
		}

		template<typename T>
		T fixMin(T const val){
			return fixMinMax(val, std::numeric_limits<T>::max());
		}

		template<typename T>
		T fixMax(T const val){
			return fixMinMax(val, std::numeric_limits<T>::min());
		}
	}


	namespace filebuilder_impl_{

		void CacheLineBuilder::operator()(StringRef const &key, uint64_t const pos){
			if (key_.equals(key))
				return;

			// store new key
			key_ = key;

			writeStr(file_, key_);
			writeU64(file_, pos);
		}



		// ==============================



		constexpr static auto MODE = std::ios::out | std::ios::binary;

		Builder::Builder(StringRef const &filename, bool const aligned) :
								file_meta(filenameMeta(filename), MODE),
								file_indx(filenameIndx(filename), MODE),
								file_line(filenameLine(filename), MODE),
								file_data(filenameData(filename), MODE),
								aligned(aligned){}

		Builder::~Builder(){
			// write the header

			uint16_t const option_aligned = aligned ? FileMetaBlob::OPTIONS_ALIGNED : FileMetaBlob::OPTIONS_NONE;

			// write table header
			uint16_t const options =
						FileMetaBlob::OPTIONS_NONE	|
						FileMetaBlob::OPTIONS_SORTED	|
						option_aligned
			;

			using hm4::disk::FileMetaBlob;

			const FileMetaBlob blob = FileMetaBlob::create(
				options,
				count,
				tombstones,
				fixMin(minCreated),
				fixMax(maxCreated)
			);

			file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));
		}

		void Builder::push_back(Pair const &pair){
			collectStats_(pair);

			// write the index
			writeU64(file_indx, index);

			// white cache line
			cacheLine(pair.getKey(), count);

			// white the data
			pair.fwrite(file_data);

			index += pair.bytes();

			if (aligned)
				index += my_align::fwriteGap(file_data, pair.bytes(), PairConf::ALIGN);

			++count;
		}

		void Builder::collectStats_(Pair const &pair){
			auto const created = pair.getCreated();

			if (created < minCreated)
				minCreated = created;

			if (created > maxCreated)
				maxCreated = created;

			if (pair.isTombstone())
				++tombstones;
		}

	} // namespace filebuilder_impl_



} // namespace

} // namespace
} // namespace hm4

