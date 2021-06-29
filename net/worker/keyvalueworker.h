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

#include "logger.h"

namespace net::worker{



	template<class Protocol, class DBAdapter, bool Mutable>
	struct KeyValueWorkerCommandStorage;



	template<class Protocol, class DBAdapter>
	class KeyValueWorkerCommandStorage<Protocol, DBAdapter, false>{
		commands::System	::Cointainer<Protocol, DBAdapter>	mod_system		;
		commands::Info		::Cointainer<Protocol, DBAdapter>	mod_info		;
		commands::Reload	::Cointainer<Protocol, DBAdapter>	mod_reload		;
		commands::Immutable	::Cointainer<Protocol, DBAdapter>	mod_immutable		;
		commands::Accumulators	::Cointainer<Protocol, DBAdapter>	mod_accumulators	;

	public:
		template<class Map>
		void registerModules(Map &m){
			mod_system		.registerModule(m);
			mod_info		.registerModule(m);
			mod_reload		.registerModule(m);
			mod_immutable		.registerModule(m);
			mod_accumulators	.registerModule(m);
		}
	};



	template<class Protocol, class DBAdapter>
	class KeyValueWorkerCommandStorage<Protocol, DBAdapter, true>{
		KeyValueWorkerCommandStorage<Protocol, DBAdapter, false> parentStorage_;

		commands::Mutable	::Cointainer<Protocol, DBAdapter>	mod_mutable	;
		commands::Counter	::Cointainer<Protocol, DBAdapter>	mod_counter	;

	public:
		template<class Map>
		void registerModules(Map &m){
			parentStorage_.registerModules(m);

			mod_mutable	.registerModule(m);
			mod_counter	.registerModule(m);
		}
	};



	// ====================================



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db) : db_(db){
			cmdStorage_->registerModules(map_);
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
		using MyBase	= commands::Base<Protocol, DBAdapter>;
		using Map	= std::unordered_map<std::string_view, const MyBase *>;
		using MyStorage	= KeyValueWorkerCommandStorage<Protocol, DBAdapter, DBAdapter::MUTABLE>;

	private:
		Protocol	protocol_;
		DBAdapter	&db_;

		// memory allocation gives stable references.
		std::unique_ptr<MyStorage>	cmdStorage_ = std::make_unique<MyStorage>();

		Map		map_;
	};



} // namespace



#endif
