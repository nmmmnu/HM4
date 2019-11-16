#ifndef DISK_FILE_FLUSH_H_
#define DISK_FILE_FLUSH_H_

#include "disk/filebuilder.h"

#include "stringreplace.h"

namespace hm4{
namespace flusher{

template<class IDGENERATOR>
class DiskFileFlush{
private:
	constexpr static char DIR_WILDCARD = '*';

public:
	template<class UIDGENERATOR>
	DiskFileFlush(
			UIDGENERATOR &&idGenerator,
			std::string path,
			bool const keepTombstones = true
		):
				idGenerator_(std::forward<UIDGENERATOR>(idGenerator)),
				path_(std::move(path)),
				keepTombstones_(keepTombstones){}

public:
	template<class It>
	bool operator()(It first, It last) const{
		if (first == last)
			return false;

		std::string const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_());

		disk::FileBuilder::build(filename, first, last, keepTombstones_, /* aligned */ true);

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

