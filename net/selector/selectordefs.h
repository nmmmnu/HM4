#ifndef _NET_STATUS_H
#define _NET_STATUS_H

#include <cstdint>

namespace net{
namespace selector{


enum class WaitStatus : uint8_t{
	OK,
	NONE,
	ERROR
};

enum class FDEvent : bool {
	READ,
	WRITE
};

enum class FDStatus : uint8_t{
	NONE,
	READ,
	WRITE,
	ERROR,
	STOP
};

struct FDResult{
	int		fd;
	FDStatus	status;

	constexpr FDResult(int const fd, FDStatus const status) : fd(fd), status(status) {}
	constexpr FDResult(FDStatus const status) : FDResult(-1, status){}
};


} // selector
} // namespace

#endif

