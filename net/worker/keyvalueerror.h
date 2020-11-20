#ifndef _KEY_VALUE_ERROR_H
#define _KEY_VALUE_ERROR_H

#include "worker/workerdefs.h"

namespace net::worker{
	namespace error{
		template<class Protocol>
		WorkerStatus error_(Protocol &protocol, IOBuffer &buffer, std::string_view const error){
			protocol.response_error(buffer, error);
			return WorkerStatus::WRITE;
		}

		template<class Protocol>
		WorkerStatus NotImplemented(Protocol &protocol, IOBuffer &buffer){
			return error_(protocol, buffer,"Not Implemented");
		}

		template<class Protocol>
		WorkerStatus BadRequest(Protocol &protocol, IOBuffer &buffer){
			return error_(protocol, buffer,"Not Implemented");
		}

		template<class Protocol>
		WorkerStatus InternalError(Protocol &protocol, IOBuffer &buffer){
			return error_(protocol, buffer,"Internal Error");
		}
	}
}


#endif

