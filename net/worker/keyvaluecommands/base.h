#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "iobuffer.h"
#include "worker/keyvalueerror.h"

namespace net::worker::commands{



	template<class Protocol, class DBAdapter>
	struct Base{
		virtual ~Base() = default;
		virtual WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const = 0;
	};



	template<class Command, class Map>
	void registerCmd(Map &m, Command const &command){
		for(auto const &key : command.cmd)
			m.emplace(key, &command);
	}



}


#endif

