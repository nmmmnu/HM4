#include "filemetablob.h"

#include <fstream>
#include <algorithm>

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

			my_align::fwriteGap(file, key.size(), HLINE_SIZE);
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


		template<class T>
		struct MinMax{
			void operator()(T const &val){
				if (val < min)
					min = val;

				if (val > max)
					max = val;
			}

			T const &getMin(T const &def = 0){
				return min == std::numeric_limits<T>::max() ? def : min;
			}

			T const &getMax(T const &def = 0){
				return max == std::numeric_limits<T>::min() ? def : max;
			}

		private:
			T min	= std::numeric_limits<T>::max();
			T max	= std::numeric_limits<T>::min();
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

			uint64_t count		= 0;
			uint64_t tombstones	= 0;

			// set min / max
			MinMax<uint64_t> createdMM;

			CacheLineBuilder cacheLine(file_line);

			std::for_each(first, last, [&, index = uint64_t{0}](const Pair &pair) mutable{
				if (!pair.isValid())
					return;

				if (pair.isTombstone()){
					if (keepTombstones == false)
						return;

					++tombstones;
				}

				createdMM( pair.getCreated() );

				// ==============================

				// write the index
				writeU64(file_indx, index);

				// white cache line
				cacheLine(pair.getKey(), count);

				/* white the data */
				pair.fwrite(file_data);

				index += pair.bytes();

				if (aligned)
					index += my_align::fwriteGap(file_data, pair.bytes(), PairConf::ALIGN);

				++count;
			});

			// now we miraculasly have the datacount.

			uint16_t const option_aligned = aligned ? FileMetaBlob::OPTIONS_ALIGNED : FileMetaBlob::OPTIONS_NONE;

			// write table header
			uint16_t const options =
						FileMetaBlob::OPTIONS_NONE	|
						FileMetaBlob::OPTIONS_SORTED	|
						option_aligned
			;

			using hm4::disk::FileMetaBlob;

			const FileMetaBlob blob = FileMetaBlob::create( options, count, tombstones, createdMM.getMin(), createdMM.getMax() );

			file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));

			return true;
		}

	} // namespace filebuilder_impl_


	template <class IT>
	bool build(StringRef const &filename,
					IT first, IT last,
					bool const keepTombstones, bool const aligned){

		constexpr auto mode = std::ios::out | std::ios::binary;

		std::ofstream fileMeta(filenameMeta(filename),	mode);
		std::ofstream fileIndx(filenameIndx(filename),	mode);
		std::ofstream fileLine(filenameLine(filename),	mode);
		std::ofstream fileData(filenameData(filename),	mode);

		using namespace filebuilder_impl_;

		return writeToFile(first, last, fileMeta, fileIndx, fileLine, fileData, keepTombstones, aligned);
	}

} // namespace

} // namespace
} // namespace hm4

