#ifndef DISK_FILE_FLUSH_H_
#define DISK_FILE_FLUSH_H_

#include "disk/filebuilder.h"

#include "stringreplace.h"

#include "unsortedlist.h"

#include "logger.h"

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

private:
	template<class List>
	bool process_(List const &list) const{
		std::string const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_());

		disk::FileBuilder::build(filename, std::begin(list), std::end(list), keepTombstones_, /* aligned */ true);

		return true;
	}

public:
	template<class List>
	bool operator()(List const &list) const{
		if (empty(list))
			return false;

		return process_(list);
	}

	bool operator()(UnsortedList &list) const{
		if (empty(list))
			return false;

		log__("Sorting data...");

		list.sort();

		return process_(list);
	}

private:
	IDGENERATOR	idGenerator_;
	std::string	path_;
	bool		keepTombstones_;
};

} // namespace flusher
} // namespace

#endif

