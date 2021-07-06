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

	using TombstoneOptions = hm4::disk::FileBuilder::TombstoneOptions;

public:
	template<class UIDGENERATOR>
	DiskFileFlush(
			UIDGENERATOR &&idGenerator,
			std::string path,
			TombstoneOptions const tombstoneOptions = TombstoneOptions::KEEP
		):
				idGenerator_(std::forward<UIDGENERATOR>(idGenerator)),
				path_(std::move(path)),
				tombstoneOptions_(tombstoneOptions){}

public:
	template<class It>
	bool operator()(It first, It last) const{
		if (first == last)
			return false;

		std::string const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_());

		disk::FileBuilder::build(filename, first, last, tombstoneOptions_, Pair::WriteOptions::ALIGNED);

		return true;
	}

private:
	IDGENERATOR		idGenerator_;
	std::string		path_;
	TombstoneOptions	tombstoneOptions_;
};

} // namespace flusher
} // namespace

#endif

