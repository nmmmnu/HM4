#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "keyvaluecommands/cmd_immutable.h"	// GET, MGET, EXISTS, TTL, STRLEN
						// HGET, HEXISTS
#include "keyvaluecommands/cmd_getx.h"		// GETX, HGETALL, DELX
						// HGETALL
#include "keyvaluecommands/cmd_accumulators.h"	// COUNT, SUM, MIN, MAX

#include "keyvaluecommands/cmd_mutable.h"	// SET, SETEX, SETNX, SETXX, EXPIRE, PERSIST, GETSET, GETDEL, DEL
						// HSET, HDEL
#include "keyvaluecommands/cmd_cas.h"		// CAS, CAD
#include "keyvaluecommands/cmd_copy.h"		// COPY, COPYNX, RENAME, RENAMENX
#include "keyvaluecommands/cmd_counter.h"	// INCR, DECR

#include "keyvaluecommands/cmd_queue.h"		// SADD, SPOP
#include "keyvaluecommands/cmd_hll.h"		// PFADD, PFCOUNT, PFINTERSECT, PFMERGE, PFBITS, PFERROR

#include "keyvaluecommands/cmd_bitset.h"	// SETBIT, GETBIT, BITCOUNT, BITMAX

#include "keyvaluecommands/cmd_info.h"		// INFO, VERSION, PING, ECHO

#include "keyvaluecommands/cmd_compat.h"	// SELECT, TYPE, TOUCH

#include "keyvaluecommands/cmd_reload.h"	// SAVE, RELOAD
#include "keyvaluecommands/cmd_system.h"	// EXIT, SHUTDOWN

#include "protocol/protocoldefs.h"

#include <memory>
#include <vector>
#include <unordered_map>

#include "logger.h"

namespace net::worker{



	namespace key_value_worker_impl_{
		template<template<class, class> class Module, class DBAdapter, class RegisterPack>
		void registerModule(RegisterPack &pack){
			using M = Module<DBAdapter, RegisterPack>;

			log__<LogLevel::NOTICE>("Loading", M::name, "module...");
			M::load(pack);
		}

		template<class DBAdapter, class RegisterPack, template<class, class> typename... Modules>
		void registerModulesAll(RegisterPack &pack){
			( registerModule<Modules, DBAdapter, RegisterPack>(pack), ...);
		}

		template<class DBAdapter, class Storage, class Map>
		void registerModules(Storage &s, Map &m){
			s.reserve(3 + 4 + 3 + 2 + 2);

			struct RegisterPack{
				Storage	&storage;
				Map	&map;
			};

			RegisterPack pack{s, m};

			using namespace commands;

			registerModulesAll<DBAdapter, RegisterPack,
				Immutable	::RegisterModule,
				Accumulators	::RegisterModule,
				GetX		::RegisterModule,

				Mutable		::RegisterModule,
				CAS		::RegisterModule,
				Copy		::RegisterModule,
				Counter		::RegisterModule,

				Queue		::RegisterModule,
				HLL		::RegisterModule,
				BITSET		::RegisterModule,

				Info		::RegisterModule,
				Compat		::RegisterModule,

				Reload		::RegisterModule,
				System		::RegisterModule
			>(pack);
		}



		template<class Protocol>
		struct Visitor{
			Protocol const	&protocol;
			IOBuffer	&buffer;

			void operator()(std::nullptr_t){
				protocol.response_ok(buffer);
			}

			void operator()(bool b){
				protocol.response_bool(buffer, b);
			}

			void operator()(int64_t number){
				response_number(number);
			}

			void operator()(uint64_t number){
				response_number(number);
			}

			void operator()(std::string_view s){
				protocol.response_string(buffer, s);
			}

			void operator()(const MySpan<std::string_view, MySpanConstructor::EXPLICIT> container){
				protocol.response_strings(buffer, container);
			}

		private:
			template<typename T>
			void response_number(T number){
				to_string_buffer_t std_buffer;

				std::string_view const val = to_string(number, std_buffer);

				this->operator()(val);
			}
		};

	}



	// ====================================



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db) : db_(db){
			using namespace key_value_worker_impl_;

			registerModules<DBAdapter>(storage_, map_);
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

			auto result = command(protocol_.getParams(), db_, *output_);

			return translate_(result, buffer);
		}

	private:
		using MyBase		= commands::Base<DBAdapter>;
		using Storage		= std::vector<std::unique_ptr<MyBase> >;
		using Map		= std::unordered_map<std::string_view, MyBase *>;

		WorkerStatus translate_(commands::Result const result, IOBuffer &buffer) const{
			buffer.clear();

			using cs = commands::Status;

			switch(result.status){
			case cs::DISCONNECT	: return WorkerStatus::DISCONNECT;
			case cs::SHUTDOWN  	: return WorkerStatus::SHUTDOWN;

			default			: // avoid warning
			case cs::ERROR		: return error::BadRequest(protocol_, buffer);

			case cs::OK		: return formatBuffer_(result.data, buffer);
			}
		}

		WorkerStatus formatBuffer_(commands::Result::ResultData const data, IOBuffer &buffer) const{
			using namespace key_value_worker_impl_;

			Visitor<Protocol> v{ protocol_, buffer };

			std::visit(v, data);

			return WorkerStatus::WRITE;
		}

	private:
		Protocol				protocol_	;
		DBAdapter				&db_		;

		Storage					storage_	;
		Map					map_		;

		std::unique_ptr<commands::OutputBlob>	output_ = std::make_unique<commands::OutputBlob>();
	};



} // namespace



#endif
