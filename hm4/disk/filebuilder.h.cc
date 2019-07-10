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


		struct Builder{
			using value_type = Pair const;

			constexpr static auto mode = std::ios::out | std::ios::binary;

			Builder(StringRef const &filename, bool const aligned) :
								file_meta(filenameMeta(filename), mode),
								file_indx(filenameIndx(filename), mode),
								file_line(filenameLine(filename), mode),
								file_data(filenameData(filename), mode),
								aligned(aligned){}

			~Builder(){
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
					getMinCreated_(),
					getMaxCreated_()
				);

				file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));
			}

			void push_back(Pair const &pair){
				collectStats(pair);

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

		private:
			void collectStats(Pair const &pair){
				auto const created = pair.getCreated();

				if (created < minCreated)
					minCreated = created;

				if (created > maxCreated)
					maxCreated = created;

				if (pair.isTombstone())
					++tombstones;
			}

			uint64_t const &getMinCreated_(uint64_t const &def = 0) const{
				return minCreated == std::numeric_limits<uint64_t>::max() ? def : minCreated;
			}

			uint64_t const &getMaxCreated_(uint64_t const &def = 0) const{
				return maxCreated == std::numeric_limits<uint64_t>::min() ? def : maxCreated;
			}

		private:
			std::ofstream	file_meta,
					file_indx,
					file_line,
					file_data;

			bool		aligned;

			CacheLineBuilder cacheLine{ file_line };

			uint64_t	index		= 0;

			uint64_t	minCreated	= std::numeric_limits<uint64_t>::max();
			uint64_t	maxCreated	= std::numeric_limits<uint64_t>::min();

			uint64_t	count		= 0;
			uint64_t	tombstones	= 0;
		};

	} // namespace filebuilder_impl_


	template <class IT>
	bool build(StringRef const &filename, IT first, IT last,
					bool const keepTombstones, bool const aligned){

		using namespace filebuilder_impl_;

		Builder builder(filename, aligned);

		std::copy_if(first, last, std::back_inserter(builder), [keepTombstones](Pair const &pair){
			if (!pair.isValid())
				return false;

			if (keepTombstones == false && pair.isTombstone())
				return false;

			return true;
		});

		return true;
	}

} // namespace

} // namespace
} // namespace hm4

