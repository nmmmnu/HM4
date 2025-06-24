#ifndef DISK_FILE_FLUSH_H_
#define DISK_FILE_FLUSH_H_

#include "disk/filebuilder.h"

#include "stringreplace.h"
#include "mybufferview.h"

#include <cassert>

namespace hm4{
namespace flusher{

template<class IDGenerator>
class DiskFileFlush{
private:
	constexpr static inline std::string_view DIR_WILDCARD = "*";

	using TombstoneOptions = hm4::disk::FileBuilder::TombstoneOptions;

public:
	template<class UIDGenerator, typename UString>
	DiskFileFlush(
			UIDGenerator		&&idGenerator,
			UString			&&path,
			TombstoneOptions	tombstoneOptions = TombstoneOptions::KEEP):
				idGenerator_		(std::forward<UIDGenerator>(idGenerator)	),
				path_			(std::forward<UString>(path)			),
				tombstoneOptions_	(tombstoneOptions				){}

public:
	template<class List>
	bool operator()(List const &list, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite) const{
		if (std::empty(list))
			return false;

		typename IDGenerator::to_string_buffer_t buffer;

		auto const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_(buffer));

		disk::FileBuilder::build(filename, buffersWrite,
							std::begin(list), std::end(list),
							tombstoneOptions_,
							Pair::WriteOptions::ALIGNED);

		return true;
	}

	template<class List>
	bool operator()(List const &list, hm4::disk::FileBuilder::FileBuilderWriteBuffers &buffersWrite, MyBuffer::ByteBufferView bufferHash) const{
		if (std::empty(list))
			return false;

		assert(bufferHash);

		typename IDGenerator::to_string_buffer_t buffer;

		auto const filename = StringReplace::replaceByCopy(path_, DIR_WILDCARD, idGenerator_(buffer));

		disk::FileBuilder::build(filename, buffersWrite,
							std::begin(list), std::end(list),
							tombstoneOptions_,
							Pair::WriteOptions::ALIGNED,
							list.size(), bufferHash);

		return true;
	}

private:
	IDGenerator			idGenerator_;
	std::string			path_;
	TombstoneOptions		tombstoneOptions_;
};

} // namespace flusher
} // namespace

#endif

