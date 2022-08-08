#ifndef DISK_FILE_FLUSH_H_
#define DISK_FILE_FLUSH_H_

#include "disk/filebuilder.h"

#include "stringreplace.h"

namespace hm4{
namespace flusher{

template<class IDGenerator>
class DiskFileFlush{
private:
	constexpr static char DIR_WILDCARD = '*';

	using TombstoneOptions = hm4::disk::FileBuilder::TombstoneOptions;

public:
	template<class UIDGenerator, typename UString>
	DiskFileFlush(
			UIDGenerator &&idGenerator,
			UString &&path,
			TombstoneOptions const tombstoneOptions = TombstoneOptions::KEEP
		):
				idGenerator_(std::forward<UIDGenerator>(idGenerator)),
				path_(std::forward<UString>(path)),
				tombstoneOptions_(tombstoneOptions){}

public:
	template<class It>
	bool operator()(It first, It last) const{
		if (first == last)
			return false;

		typename IDGenerator::to_string_buffer_t buffer;

		auto const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_(buffer));

		disk::FileBuilder::build(filename, first, last, tombstoneOptions_, Pair::WriteOptions::ALIGNED);

		return true;
	}

private:
	IDGenerator		idGenerator_;
	std::string		path_;
	TombstoneOptions	tombstoneOptions_;
};

} // namespace flusher
} // namespace

#endif

