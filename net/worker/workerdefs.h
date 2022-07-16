#ifndef _WORKER_DEFS_H
#define _WORKER_DEFS_H

#include <cstdint>

namespace net::worker{



enum class WorkerStatus : uint8_t{
	PASS,
	READ,
	WRITE,
	DISCONNECT,
	DISCONNECT_ERROR,
	SHUTDOWN
};



namespace error{
	template<class Protocol, class Buffer>
	WorkerStatus error_(Protocol &protocol, Buffer &buffer, std::string_view const error){
		protocol.response_error(buffer, error);
		return WorkerStatus::WRITE;
	}

	template<class Protocol, class Buffer>
	WorkerStatus NotImplemented(Protocol &protocol, Buffer &buffer){
		return error_(protocol, buffer,"Not Implemented");
	}

	template<class Protocol, class Buffer>
	WorkerStatus BadRequest(Protocol &protocol, Buffer &buffer){
		return error_(protocol, buffer,"Not Implemented");
	}

	template<class Protocol, class Buffer>
	WorkerStatus InternalError(Protocol &protocol, Buffer &buffer){
		return error_(protocol, buffer,"Internal Error");
	}
}


} // namespace

#endif

