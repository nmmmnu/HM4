#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "ilist.h"



#include "keyvaluecommands/cmd_immutable.h"		// GET, MGET, EXISTS, TTL, STRLEN, GETRANGE
							// HGET, HEXISTS
#include "keyvaluecommands/cmd_immutable_x.h"		// GETX, GETXR
							// HGETALL, HGETKEYS, HGETVALS
#include "keyvaluecommands/cmd_accumulators_x.h"	// COUNTX, COUNTXR, SUMX, SUMXR, MINX, MINXR, MAXX, MAXXR

#include "keyvaluecommands/cmd_mutable_get.h"		// GETSET, GETDEL

#include "keyvaluecommands/cmd_mutable.h"		// SET, SETEX, SETNX, SETXX, APPEND, EXPIRE, PERSIST, DEL
							// HSET, HDEL
#include "keyvaluecommands/cmd_mutable_x.h"		// DELX, DELXR, EXPIREX, EXPIREXR, PERSISTX, PERSISTXR
							// HDELALL, HPERSISTALL, HEXPIREALL
#include "keyvaluecommands/cmd_cas.h"			// CAS, CAD
#include "keyvaluecommands/cmd_copy.h"			// COPY, COPYNX, RENAME, RENAMENX
#include "keyvaluecommands/cmd_counter.h"		// INCR, DECR

#include "keyvaluecommands/cmd_queue.h"			// SADD, SPOP
#include "keyvaluecommands/cmd_bitset.h"		// SETBIT, GETBIT, BITCOUNT, BITMAX
#include "keyvaluecommands/cmd_hll.h"			// PFADD, PFCOUNT, PFINTERSECT, PFMERGE, PFBITS, PFERROR
#include "keyvaluecommands/cmd_bf.h"			// BFADD, BFRESERVE, BFEXISTS, BFMEXISTS
#include "keyvaluecommands/cmd_cbf.h"			// CBFADD, CBFADDCOUNT, CBFRESERVE, CBFCOUNT, CBFMCOUNT
#include "keyvaluecommands/cmd_cms.h"			// CMSADD, CMSADDCOUNT, CMSRESERVE, CMSCOUNT, CMSMCOUNT
#include "keyvaluecommands/cmd_hh.h"			// HHINCR, HHDECR, HHRESERVE, HHGET
#include "keyvaluecommands/cmd_mg.h"			// MGADD, MGRESERVE, MGGET
#include "keyvaluecommands/cmd_rs.h"			// RSADD, RSRESERVE, RSGET, RSGETCOUNT
#include "keyvaluecommands/cmd_geo.h"			// GEOADD, GEOREM, GEOGET, GEOMGET, GEOENCODE, GEODECODE
#include "keyvaluecommands/cmd_mortoncurve.h"		// MC2GET, MC2MGET, MC2EXISTS, MC2SCORE, MC2ADD, MC2REM, MC2POINT, MC2RANGENAIVE, MC2RANGE, MC2ENCODE, MC2DECODE
#include "keyvaluecommands/cmd_linearcurve.h"		// MC1GET, MC1MGET, MC1EXISTS, MC1SCORE, MC1ADD, MC1REM, MC1POINT, MC1RANGE
#include "keyvaluecommands/cmd_tdigest.h"		// TDADD, TDRESERVE

#include "keyvaluecommands/cmd_index.h"

#include "keyvaluecommands/cmd_info.h"			// INFO, DBSIZE, VERSION, MAXKEYSIZE, MAXVALSIZE, PING, ECHO

#include "keyvaluecommands/cmd_compat.h"		// SELECT, TYPE, TOUCH

#include "keyvaluecommands/cmd_murmur.h"		// MURMUR

#include "keyvaluecommands/cmd_reload.h"		// SAVE, RELOAD
#include "keyvaluecommands/cmd_system.h"		// EXIT, SHUTDOWN

#include "keyvaluecommands/cmd_test.h"			// TEST

#include "protocol/protocoldefs.h"

#include <vector>
#include <unordered_map>

#include "logger.h"

namespace net::worker{



	namespace key_value_worker_impl_{
		template<template<class, class, class> class Module, class Protocol, class DBAdapter, class RegisterPack>
		void registerModule(RegisterPack &pack){
			using M = Module<Protocol, DBAdapter, RegisterPack>;

			logger<Logger::STARTUP>() << "Loading" << M::name << "module...";
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
				MutableGET	::RegisterModule,
				MutableX	::RegisterModule,
				CAS		::RegisterModule,
				Copy		::RegisterModule,
				Counter		::RegisterModule,

				Queue		::RegisterModule,
				BITSET		::RegisterModule,
				HLL		::RegisterModule,
				BF		::RegisterModule,
				CBF		::RegisterModule,
				CMS		::RegisterModule,
				HH		::RegisterModule,
				RS		::RegisterModule,
				MG		::RegisterModule,
				Geo		::RegisterModule,
				MortonCurve	::RegisterModule,
				LinearCurve	::RegisterModule,
				TDigest		::RegisterModule,

				Index		::RegisterModule,

				Murmur		::RegisterModule,

				Info		::RegisterModule,
				Compat		::RegisterModule,

				Reload		::RegisterModule,
				System		::RegisterModule,

				Test		::RegisterModule
			>(pack);
		}
	}



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		constexpr static bool LogCommands	= false;
		constexpr static auto LogCommandsLevel	= Logger::DEBUG;

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
				return emitInternalError(protocol_, buffer);

			// FETCH command

			const auto &p = protocol_.getParams();

			if (p.empty())
				return emitError(protocol_, buffer, ResultErrorMessages::SYS_PROTOCOL_BREAK);

			// EXEC command

			auto it = map_.find(p.front());

			if (it == std::end(map_))
				return emitError(protocol_, buffer, ResultErrorMessages::SYS_NOT_IMPLEMENTED);

			// LOG command
			if constexpr(LogCommands)
				logger<LogCommandsLevel>().range(std::begin(p), std::end(p));

			auto &command = *it->second;

			commands::Result<Protocol> result{ protocol_, output_buffer_ };

			output_.resetAllocator();

			command(protocol_.getParams(), db_, result, output_);

			return translate_(result, buffer);
		}

		template<class ...Args>
		void connection_notify(Args &&...args){
			db_.connection_notify(std::forward<Args>(args)...);
		}

	private:
		using MyBase		= commands::Base<Protocol, DBAdapter>;
		using Storage		= std::vector<std::unique_ptr<MyBase> >;
		using Map		= std::unordered_map<std::string_view, MyBase *>;

		WorkerStatus translate_(commands::Result<Protocol> const result, IOBuffer &buffer){
			using cs = commands::Status;

			if (output_buffer_.size() == 0){
				return emitError(protocol_, buffer, ResultErrorMessages::SYS_UNHANDLED);
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
		Protocol		protocol_	;
		DBAdapter		&db_		;

		Storage			storage_	;
		Map			map_		;

		commands::OutputBlob	output_		;

		IOBuffer		output_buffer_	;

	private:
		static_assert(Protocol::MAX_PARAMS <= commands::OutputBlob::ContainerSize	);
		static_assert(Protocol::MAX_PARAMS <= commands::OutputBlob::ParamContainerSize	);
	};



} // namespace



#endif
