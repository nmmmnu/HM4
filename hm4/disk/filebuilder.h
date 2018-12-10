#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include "disk/filenames.h"

namespace hm4{
namespace disk{


namespace FileBuilder{
	template <class IT>
	static bool build(StringRef const &filename,
				IT first, IT last,
				bool keepTombstones, bool aligned);

	template <class List>
	static bool build(StringRef const &filename,
				List const &list,
				bool const keepTombstones, bool const aligned){
		return build(filename, std::begin(list), std::end(list), keepTombstones, aligned);
	}
}


} // namespace
} // namespace hm4


#include "filebuilder.h.cc"

#endif

