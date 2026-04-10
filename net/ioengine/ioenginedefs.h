#ifndef _NET_IOENGINE_DEFS_H
#define _NET_IOENGINE_DEFS_H

#include <cstdint>

namespace net{
namespace ioengine{



enum class WaitStatus : uint8_t{
	OK,
	NONE,
	ERROR
};

enum class FDOperation : uint8_t{
	NONE	,
	ACCEPT	,
	CLOSE	,
	READ	,
	WRITE
//	ERROR
};

struct FDEvent{
	constexpr FDEvent(int fd, int result, FDOperation op) :
						fd	(fd	),
						result	(result	),
						op	(op	){}

	int		fd;
	int		result;
	FDOperation	op;
};

} // namespace ioengine
} // namespace

#endif

