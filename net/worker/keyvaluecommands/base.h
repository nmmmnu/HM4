#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "iobuffer.h"
#include "worker/keyvalueerror.h"

namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct cmd_base{
		virtual ~cmd_base() = default;
		virtual WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const = 0;
	};



}


#endif

