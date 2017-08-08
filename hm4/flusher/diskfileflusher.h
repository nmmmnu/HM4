#ifndef _DISK_FILE_FLUSH_H
#define _DISK_FILE_FLUSH_H

#include "disk/filebuilder.h"

#include "stringreplace.h"

// ==============================

namespace hm4{
namespace flusher{


template<class IDGENERATOR>
class DiskFileFlusher{
private:
	constexpr static char DIR_WILDCARD = '*';

public:
	template<class UIDGENERATOR>
	DiskFileFlusher(
			UIDGENERATOR &&idGenerator,
			const StringRef &path,
			bool const keepTombstones = true
		):
				idGenerator_(std::forward<UIDGENERATOR>(idGenerator)),
				path_(path),
				keepTombstones_(keepTombstones){}

public:
	template<class LIST>
	bool operator << (const LIST &list) const{
		if (list.empty())
			return false;

		std::cout << "Flushing data to disk..." << '\n';

		//const std::string &filename = StringRef::concatenate({ path_, idGenerator_(), ext_ });

		StringReplaceCopy str_replace;
		const std::string &filename = str_replace(path_, DIR_WILDCARD, idGenerator_());

		using FileBuilder = disk::FileBuilder;

		FileBuilder fb;
		fb.build(filename, list, keepTombstones_, /* aligned */ true);

		return true;
	}

private:
	IDGENERATOR	idGenerator_;
	std::string	path_;
	bool		keepTombstones_;
};

} // namespace flusher
} // namespace

#endif

