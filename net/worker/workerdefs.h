#ifndef _WORKER_DEFS_H
#define _WORKER_DEFS_H

#include <cstdint>

namespace net{
namespace worker{


enum class WorkerStatus : uint8_t{
	PASS,
	READ,
	WRITE,
	DISCONNECT,
	DISCONNECT_ERROR,
	SHUTDOWN
};


} // namespace worker
} // namespace

#endif

