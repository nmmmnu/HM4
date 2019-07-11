#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include <fstream>
#include <algorithm>
#include <limits>

#include "pair.h"

namespace hm4{
namespace disk{
namespace FileBuilder{

	namespace filebuilder_impl_{



		class CacheLineBuilder{
		private:
			constexpr static auto HLINE_SIZE = PairConf::HLINE_SIZE;

		public:
			CacheLineBuilder(std::ofstream &file) : file_(file){}

			~CacheLineBuilder();

			void operator()(StringRef const &current, uint64_t const pos);

		private:
			void store_();

		private:
			SmallString<HLINE_SIZE>	key_;
			uint64_t		pos_	= 0;
			std::ofstream		&file_;
		};

		// ==============================

		struct Builder{
			using value_type = Pair const;

			Builder(StringRef const &filename, bool const aligned);

			~Builder();

			void push_back(Pair const &pair);

		private:
			void collectStats_(Pair const &pair);

			uint64_t getMinCreated_() const{
				return getMM__<std::numeric_limits<uint64_t>::max()>(minCreated);
			}

			uint64_t getMaxCreated_() const{
				return getMM__<std::numeric_limits<uint64_t>::min()>(maxCreated);
			}

			template<uint64_t limit>
			static uint64_t getMM__(uint64_t const val, uint64_t const fallback = 0){
				return val == limit ? fallback : val;
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

	} // namespace



	template <class IT>
	bool build(StringRef const &filename, IT first, IT last,
					bool const keepTombstones, bool const aligned){

		using namespace filebuilder_impl_;

		Builder builder(filename, aligned);

		std::copy_if(first, last, std::back_inserter(builder), [keepTombstones](Pair const &pair){
			// invalid pairs must be kept as tombstones
			if (keepTombstones)
				return true;

			if (!pair.isValid())
				return false;

			if (pair.isTombstone())
				return false;

			return true;
		});

		return true;
	}



} // namespace
} // namespace
} // namespace hm4


#endif

