#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "iobuffer.h"
#include "worker/workerdefs.h"

namespace net{
namespace worker{
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



	template<class Protocol, class DBAdapter>
	struct cmd_base{
		virtual ~cmd_base() = default;
		virtual WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) = 0;
	};



}
}


#endif

