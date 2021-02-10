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
	struct KeyValueWorkerCommandStorage<Protocol, DBAdapter, false>{
		cmd_EXIT	<Protocol, DBAdapter> exit	;
		cmd_SHUTDOWN	<Protocol, DBAdapter> shutdown	;
		cmd_INFO	<Protocol, DBAdapter> info	;
		cmd_SAVE	<Protocol, DBAdapter> save	;
		cmd_RELOAD	<Protocol, DBAdapter> reload	;
		cmd_GET		<Protocol, DBAdapter> get	;
		cmd_GETX	<Protocol, DBAdapter> getx	;
		cmd_COUNT	<Protocol, DBAdapter> count	;
		cmd_SUM		<Protocol, DBAdapter> sum	;
		cmd_MIN		<Protocol, DBAdapter> min	;
		cmd_MAX		<Protocol, DBAdapter> max	;

		template<class Map>
		void populateMap(Map &m){
			auto r = [&m](auto const &t){
				for(auto const &key : t.cmd)
					m.emplace(key, &t);
			};

			r(exit		);
			r(shutdown	);
			r(info		);
			r(save		);
			r(reload	);
			r(get		);
			r(getx		);
			r(count		);
			r(sum		);
			r(min		);
			r(max		);
		}
	};

	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerCommandStorage<Protocol, DBAdapter, true>{
		KeyValueWorkerCommandStorage<Protocol, DBAdapter, false> parentStorage_;

		cmd_SET		<Protocol, DBAdapter> set	;
		cmd_SETEX	<Protocol, DBAdapter> setex	;
		cmd_DEL		<Protocol, DBAdapter> del	;
		cmd_GETSET	<Protocol, DBAdapter> getset	;
		cmd_INCR	<Protocol, DBAdapter> incr	;
		cmd_DECR	<Protocol, DBAdapter> decr	;

		template<class Map>
		void populateMap(Map &m){
			parentStorage_.populateMap(m);

			auto r = [&m](auto const &t){
				for(auto const &key : t.cmd)
					m.emplace(key, &t);
			};

			r(set		);
			r(setex		);
			r(del		);
			r(getset	);
			r(incr		);
			r(decr		);
		}
	};



	// ====================================



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db) : db_(db){
			cmdStorage_->populateMap(map_);
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
		using my_cmd_base	= cmd_base<Protocol, DBAdapter>;
		using Map		= std::unordered_map<std::string_view, const my_cmd_base *>;
		using MyStorage		= KeyValueWorkerCommandStorage<Protocol, DBAdapter, DBAdapter::MUTABLE>;

	private:
		Protocol	protocol_;
		DBAdapter	&db_;

		// memory allocation gives stable references.
		std::unique_ptr<MyStorage>	cmdStorage_ = std::make_unique<MyStorage>();

		Map		map_;
	};



} // namespace



#endif
