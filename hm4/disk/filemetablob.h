#ifndef DISK_FILE_META_BLOB_H_
#define DISK_FILE_META_BLOB_H_

#include <cstdint>
#include <type_traits>	// is_pod

namespace hm4{
namespace disk{

struct FileMetaBlob{
	char		logo[8];	// 8

	uint16_t	version;	// 2
	uint16_t	options;	// 2
	uint32_t	created;	// 4

	uint64_t	size;		// 8
	uint64_t	tombstones;	// 8
	uint64_t	createdMin;	// 8
	uint64_t	createdMax;	// 8

public:
	constexpr static std::size_t	ALIGN		= sizeof(uint64_t);

public:
	static constexpr const char	*LOGO		= "ZUSE004";
	static constexpr uint16_t	VERSION		= 4;

	static constexpr uint16_t	OPTIONS_NONE	= 0;
	static constexpr uint16_t	OPTIONS_SORTED	= 1 << 0;
	static constexpr uint16_t	OPTIONS_ALIGNED	= 1 << 1;

public:
	static FileMetaBlob create(uint16_t options, uint64_t count, uint64_t tombstones, uint64_t createdMin, uint64_t createdMax);

} __attribute__((__packed__));

static_assert(std::is_pod<FileMetaBlob>::value, "FileMetaBlob must be POD type");

} // namespace
} // namespace hm4

#endif

