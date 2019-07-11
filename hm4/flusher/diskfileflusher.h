#ifndef DISK_FILE_FLUSH_H_
#define DISK_FILE_FLUSH_H_

#include "disk/filebuilder.h"

#include "stringreplace.h"


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
			std::string path,
			bool const keepTombstones = true
		):
				idGenerator_(std::forward<UIDGENERATOR>(idGenerator)),
				path_(std::move(path)),
				keepTombstones_(keepTombstones){}

public:
	template<class LIST>
	bool operator << (const LIST &list) const{
		if (empty(list))
			return false;

		//const std::string &filename = StringRef::concatenate({ path_, idGenerator_(), ext_ });

		const std::string &filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_());

		disk::FileBuilder::build(filename, std::begin(list), std::end(list), keepTombstones_, /* aligned */ true);

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

