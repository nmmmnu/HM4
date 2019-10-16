#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include <fstream>
#include <algorithm>
#include <limits>

#include "pair.h"

namespace hm4{
namespace disk{
namespace FileBuilder{

	template <class IT>
	bool build(std::string_view const filename, IT first, IT last,
					bool const keepTombstones, bool const aligned);

	// ==============================

	namespace filebuilder_impl_{

		class CacheLineBuilder{
		private:
			constexpr static auto HLINE_SIZE = PairConf::HLINE_SIZE;

		public:
			CacheLineBuilder(std::ofstream &file) : file_(file){}

			void operator()(std::string_view const current, uint64_t const pos);

		private:
			SmallString<HLINE_SIZE>	key_;
			std::ofstream		&file_;
		};

		// ==============================

		struct Builder{
			using value_type = Pair const;

			Builder(std::string_view const filename, bool const aligned);

			~Builder();

			void push_back(Pair const &pair);

		private:
			void collectStats_(Pair const &pair);

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

	// ==============================

	template <class IT>
	bool build(std::string_view const filename, IT first, IT last,
					bool const keepTombstones, bool const aligned){

		using namespace filebuilder_impl_;

		Builder builder(filename, aligned);

		if (keepTombstones){

			// invalid pairs must be kept as tombstones
			std::copy(first, last, std::back_inserter(builder));

		}else{

			std::copy_if(first, last, std::back_inserter(builder), [](Pair const &pair){
				if (!pair.isValid())
					return false;

				if (pair.isTombstone())
					return false;

				return true;
			});

		}

		return true;
	}

} // namespace
} // namespace
} // namespace hm4


#endif

