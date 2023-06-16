#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "keyvaluecommands/cmd_immutable.h"	// GET, MGET, EXISTS, TTL, STRLEN, GETRANGE
						// HGET, HEXISTS
#include "keyvaluecommands/cmd_immutable_x.h"	// GETX, HGETALL
						// HGETALL, HGETKEYS, HGETVALS
#include "keyvaluecommands/cmd_accumulators.h"	// COUNT, SUM, MIN, MAX

#include "keyvaluecommands/cmd_mutable.h"	// SET, SETEX, SETNX, SETXX, APPEND, EXPIRE, PERSIST, GETSET, GETDEL, DEL
						// HSET, HDEL
#include "keyvaluecommands/cmd_mutable_x.h"	// DELX, EXPIREX, PERSISTX
#include "keyvaluecommands/cmd_cas.h"		// CAS, CAD
#include "keyvaluecommands/cmd_copy.h"		// COPY, COPYNX, RENAME, RENAMENX
#include "keyvaluecommands/cmd_counter.h"	// INCR, DECR

#include "keyvaluecommands/cmd_queue.h"		// SADD, SPOP
#include "keyvaluecommands/cmd_bitset.h"	// SETBIT, GETBIT, BITCOUNT, BITMAX
#include "keyvaluecommands/cmd_hll.h"		// PFADD, PFCOUNT, PFINTERSECT, PFMERGE, PFBITS, PFERROR
#include "keyvaluecommands/cmd_bf.h"		// BFADD, BFRESERVE, BFEXISTS, BFMEXISTS
#include "keyvaluecommands/cmd_cms.h"		// CMSADD, CMSRESERVE, CMSCOUNT, CMSMCOUNT
#include "keyvaluecommands/cmd_array.h"		// CVSET, CVGET, CVMGET, CVMAX

#include "keyvaluecommands/cmd_info.h"		// INFO, DBSIZE, VERSION, MAXKEYSIZE, MAXVALSIZE, PING, ECHO

#include "keyvaluecommands/cmd_compat.h"	// SELECT, TYPE, TOUCH

#include "keyvaluecommands/cmd_murmur.h"	// MURMUR

#include "keyvaluecommands/cmd_reload.h"	// SAVE, RELOAD
#include "keyvaluecommands/cmd_system.h"	// EXIT, SHUTDOWN

#include "protocol/protocoldefs.h"

#include <memory>
#include <vector>
#include <unordered_map>

#include "logger.h"

namespace net::worker{



	namespace key_value_worker_impl_{
		template<template<class, class, class> class Module, class Protocol, class DBAdapter, class RegisterPack>
		void registerModule(RegisterPack &pack){
			using M = Module<Protocol, DBAdapter, RegisterPack>;

			getLogger().startup() << "Loading" << M::name << "module...";
			M::load(pack);
		}

		template<class Protocol, class DBAdapter, class RegisterPack, template<class, class, class> typename... Modules>
		void registerModulesAll(RegisterPack &pack){
			pack.storage.reserve(sizeof...(Modules));

			( registerModule<Modules, Protocol, DBAdapter, RegisterPack>(pack), ...);
		}

		template<class Protocol, class DBAdapter, class Storage, class Map>
		void registerModules(Storage &s, Map &m){
			struct RegisterPack{
				Storage	&storage;
				Map	&map;
			};

			RegisterPack pack{s, m};

			using namespace commands;

			registerModulesAll<Protocol, DBAdapter, RegisterPack,
				Immutable	::RegisterModule,
				ImmutableX	::RegisterModule,
				Accumulators	::RegisterModule,

				Mutable		::RegisterModule,
				MutableX	::RegisterModule,
				CAS		::RegisterModule,
				Copy		::RegisterModule,
				Counter		::RegisterModule,

				Queue		::RegisterModule,
				BITSET		::RegisterModule,
				HLL		::RegisterModule,
				BF		::RegisterModule,
				CMS		::RegisterModule,
				CV		::RegisterModule,

				Murmur		::RegisterModule,

				Info		::RegisterModule,
				Compat		::RegisterModule,

				Reload		::RegisterModule,
				System		::RegisterModule
			>(pack);
		}
	}



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db, size_t output_buffer_reserve) :
							db_(db),
							output_buffer_(output_buffer_reserve){

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

			commands::Result<Protocol> result{ protocol_, output_buffer_ };

			command(protocol_.getParams(), db_, result, *output_);

			return translate_(result, buffer);
		}

		void connection_notify(uint64_t conn, uint64_t spare){
			db_.connection_notify(conn, spare);
		}

	private:
		using MyBase		= commands::Base<Protocol, DBAdapter>;
		using Storage		= std::vector<std::unique_ptr<MyBase> >;
		using Map		= std::unordered_map<std::string_view, MyBase *>;

		WorkerStatus translate_(commands::Result<Protocol> const result, IOBuffer &buffer){
			using cs = commands::Status;

			if (output_buffer_.size() == 0){
				return error::BadRequest(protocol_, buffer);
			}

			switch(result.getStatus()){
			case cs::DISCONNECT	: return WorkerStatus::DISCONNECT;
			case cs::SHUTDOWN  	: return WorkerStatus::SHUTDOWN;
			case cs::OK		: break;
			}

			using std::swap;
			swap(output_buffer_, buffer);

			return WorkerStatus::WRITE;
		}

	private:
		Protocol				protocol_	;
		DBAdapter				&db_		;

		Storage					storage_	;
		Map					map_		;

		std::unique_ptr<commands::OutputBlob>	output_ = std::make_unique<commands::OutputBlob>();

		IOBuffer				output_buffer_	;
	};



} // namespace



#endif
