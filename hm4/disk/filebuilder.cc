#include "filebuilder.h"

#include "filemetablob.h"

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

		void writeU64(FileWriter &file, uint64_t const data){
			uint64_t const be_data = htobe(data);

			file.write( & be_data, sizeof(uint64_t));
		}

		void writeStr(FileWriter &file, HPair::HKey const data){
			file.write( & data, sizeof(HPair::HKey));
		}
	}



	void FileDataBuilder::operator()(Pair const &pair){
		pair.fwrite(file_data, fileWriteOptions);
	}



	void FileIndxBuilder::operator()(Pair const &pair){
		writeU64(file_indx, index);

		if (fileWriteOptions == Pair::WriteOptions::ALIGNED)
			index += my_align::calc(pair.bytes(), PairConf::ALIGN);
		else
			index += pair.bytes();
	}



	void FileLineBuilder::operator()(Pair const &pair){
		auto hkey = HPair::SS::create<HPair::SS::ByteOrder::BigEndian>(pair.getKey());

		if (hkey_ != hkey){
			// store new key
			hkey_ = hkey;

			writeStr(file_line, hkey_);
			writeU64(file_line, pos);
		}

		++pos;
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

	FileMetaBuilder::~FileMetaBuilder(){
		// write the header
		uint16_t const option_aligned = fileWriteOptions == Pair::WriteOptions::ALIGNED ? FileMetaBlob::OPTIONS_ALIGNED : FileMetaBlob::OPTIONS_NONE;

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

		auto const filename_temp = filenameMetaTemp(filename);
		auto const filename_norm = filenameMeta_string_view(filename);

		{
			MyBuffer::ByteBufferView empty; // remove me
			FileWriter file_meta{ filename_temp, empty };
			file_meta.write( & blob, sizeof(FileMetaBlob));
		}

		// "weird rename" in lock free environment.
		rename(filename_temp.data(), filename_norm.data());
	}



} // namespace FileBuilder
} // namespace disk
} // namespace hm4

