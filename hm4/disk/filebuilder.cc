#include "filebuilder.h"

#include "filemetablob.h"

#include "myalign.h"
#include "myendian.h"

namespace hm4{
namespace disk{
namespace FileBuilder{

	namespace{
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

		void writeU64(std::ofstream &file, uint64_t const data){
			uint64_t const be_data = htobe(data);

			file.write( (const char *) & be_data, sizeof(uint64_t));
		}

		void writeStr(std::ofstream &file, HPair::HKey const data){
			file.write( (const char *) & data, sizeof(HPair::HKey));
		}
	}



	void FileDataBuilder::operator()(Pair const &pair){
		pair.fwrite(file_data);
		if (aligned)
			my_align::fwriteGap(file_data, pair.bytes(), PairConf::ALIGN);
	}



	void FileIndxBuilder::operator()(Pair const &pair){
		writeU64(file_indx, index);

		writeLine_(pair.getKey());

		if (aligned)
			index += my_align::calc(pair.bytes(), PairConf::ALIGN);
		else
			index += pair.bytes();
	}

	void FileIndxBuilder::writeLine_(std::string_view const key){
		auto hkey = HPair::SS::createBE(key);

		if (hkey_ == hkey)
			return;

		// store new key
		hkey_ = hkey;

		writeStr(file_line, hkey_);
		writeU64(file_line, index);
	}



	FileMetaBuilder::~FileMetaBuilder(){
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

	void FileMetaBuilder::operator()(Pair const &pair){
		auto const created = pair.getCreated();

		if (created < minCreated)
			minCreated = created;

		if (created > maxCreated)
			maxCreated = created;

		if (pair.isTombstone())
			++tombstones;

		++count;
	}



} // namespace FileBuilder
} // namespace disk
} // namespace hm4

