#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "keyvaluecommands/cmd_immutable.h"	// GET, TTL
#include "keyvaluecommands/cmd_accumulators.h"	// COUNT, SUM, MIN, MAX
#include "keyvaluecommands/cmd_getx.h"		// GETX

#include "keyvaluecommands/cmd_mutable.h"	// SET, SETEX, SETNX, DEL, GETSET, EXPIRE
#include "keyvaluecommands/cmd_counter.h"	// INCR, DECR

#include "keyvaluecommands/cmd_info.h"		// INFO
#include "keyvaluecommands/cmd_reload.h"	// SAVE, RELOAD
#include "keyvaluecommands/cmd_system.h"	// EXIT, SHUTDOWN

#include "protocol/protocoldefs.h"

#include <memory>
#include <vector>
#include <unordered_map>

//#define log__(...) /* nada */
#include "logger.h"

namespace net::worker{



	namespace key_value_worker_impl_{
		template<template<class, class> class Module, class Protocol, class DBAdapter, class Storage, class Map>
		void registerModule(Storage &s, Map &m){
			using M = Module<Protocol, DBAdapter>;

			log__("Loading", M::name, "module...");
			M::load(s, m);
		}

		template<class Protocol, class DBAdapter, class Storage, class Map, template<class, class> typename... Modules>
		void registerModulesAll(Storage &s, Map &m){
			( registerModule<Modules, Protocol, DBAdapter>(s, m), ...);
		}

		template<class Protocol, class DBAdapter, class Storage, class Map>
		void registerModules(Storage &s, Map &m){
			s.reserve(2 + 2 + 3);

			using namespace commands;

			registerModulesAll<Protocol, DBAdapter, Storage, Map,
				Immutable	::RegisterModule,
				Accumulators	::RegisterModule,
				GetX		::RegisterModule,

				Mutable		::RegisterModule,
				Counter		::RegisterModule,

				Info		::RegisterModule,
				Reload		::RegisterModule,
				System		::RegisterModule
			>(s, m);
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

			auto result = command(
						protocol_,
						commands::ParamContainer{ protocol_.getParams() },
						db_,
						buffer
			);

			return translate_(result.status, buffer);
		}

	private:
		using MyBase		= commands::Base<Protocol, DBAdapter>;
		using Storage		= std::vector<std::unique_ptr<const MyBase> >;
		using Map		= std::unordered_map<std::string_view, const MyBase *>;

		WorkerStatus translate_(commands::Status const status, IOBuffer &buffer) const{
			using cs = commands::Status;
			using ws = WorkerStatus;

			switch(status){
			case cs::DISCONNECT	: return ws::DISCONNECT;
			case cs::SHUTDOWN  	: return ws::SHUTDOWN;

			default			: // avoid warning
			case cs::ERROR		: return error::BadRequest(protocol_, buffer);

			case cs::OK		: return ws::WRITE;
			}
		}

	private:
		Protocol	protocol_;
		DBAdapter	&db_;

		Storage		storage_;
		Map		map_;
	};



} // namespace



#endif
