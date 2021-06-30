#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "keyvaluecommands/cmd_system.h"	// EXIT, SHUTDOWN
#include "keyvaluecommands/cmd_reload.h"	// SAVE, RELOAD
#include "keyvaluecommands/cmd_info.h"		// INFO
#include "keyvaluecommands/cmd_immutable.h"	// GET
#include "keyvaluecommands/cmd_accumulators.h"	// GETX, COUNT, SUM, MIN, MAX
#include "keyvaluecommands/cmd_mutable.h"	// SET, SETEX, DEL, GETSET
#include "keyvaluecommands/cmd_counter.h"	// INCR, INCRBY, DECR, DECRBY

#include "protocol/protocoldefs.h"

#include <memory>
#include <unordered_map>

namespace net::worker{



	namespace key_value_worker_impl_{

		template<class Protocol, class DBAdapter, class Storage, class Map>
		void registerModules(Storage &s, Map &m){
			s.reserve(5 + 2);

			commands::System	::registerModule<Protocol, DBAdapter>(s, m);
			commands::Info		::registerModule<Protocol, DBAdapter>(s, m);
			commands::Reload	::registerModule<Protocol, DBAdapter>(s, m);
			commands::Immutable	::registerModule<Protocol, DBAdapter>(s, m);
			commands::Accumulators	::registerModule<Protocol, DBAdapter>(s, m);

			commands::Mutable	::registerModule<Protocol, DBAdapter>(s, m);
			commands::Counter	::registerModule<Protocol, DBAdapter>(s, m);
		}

	}



	// ====================================



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db) : db_(db){
			using namespace key_value_worker_impl_;

			registerModules<Protocol, DBAdapter>(storage_, map_);
		}

		WorkerStatus operator()(IOBuffer &buffer){
			using Status  = protocol::ProtocolStatus;

			// PASS

			if (buffer.size() == 0)
				return WorkerStatus::PASS;

			const Status status = protocol_( std::string_view{ buffer } );

			if (status == Status::BUFFER_NOT_READ)
				return WorkerStatus::PASS;

			buffer.clear();

			// ERROR

			if (status == Status::ERROR)
				return error::InternalError(protocol_, buffer);

			// FETCH command

			const auto &p = protocol_.getParams();

			if (p.empty())
				return error::BadRequest(protocol_, buffer);

			// EXEC command

			auto it = map_.find(p.front());

			if (it == std::end(map_))
				return error::NotImplemented(protocol_, buffer);

			auto &command = *it->second;

			return command(protocol_, db_, buffer);
		}

	private:
		using MyBase		= commands::Base<Protocol, DBAdapter>;
		using Storage		= std::vector<std::unique_ptr<const MyBase> >;
		using Map		= std::unordered_map<std::string_view, const MyBase *>;

	private:
		Protocol	protocol_;
		DBAdapter	&db_;

		Storage		storage_;
		Map		map_;
	};



} // namespace



#endif
