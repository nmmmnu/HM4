#ifndef DISK_FILE_BUILDER_H_
#define DISK_FILE_BUILDER_H_

#include "disk/filenames.h"

namespace hm4{
namespace disk{


struct FileBuilder{
	template <class LIST>
	static bool build(const StringRef &filename,
				const LIST &list,
				bool keepTombstones, bool const aligned){
		return build(filename, list.begin(), list.end(), keepTombstones, aligned);
	}

	template <class ITERATOR>
	static bool build(const StringRef &filename,
				const ITERATOR &begin, const ITERATOR &end,
				bool keepTombstones, bool const aligned);

};


} // namespace
} // namespace hm4


#include "filebuilder.h.cc"

#endif

