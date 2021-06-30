#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "iobuffer.h"
#include "worker/keyvalueerror.h"

#include <memory>

namespace net::worker::commands{



	template<class Protocol, class DBAdapter>
	struct Base{
		virtual ~Base() = default;
		virtual WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer) const = 0;
	};



	template<class Command, class Storage, class Map>
	void registerCmd(Storage &s, Map &m){
		const auto &up = s.emplace_back(std::make_unique<Command>());

		for(auto const &key : Command::cmd)
			m.emplace(key, up.get());
	}



}


#endif

