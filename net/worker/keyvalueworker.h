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

#include <unordered_map>

namespace net::worker{



	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerCommandMapBase{
		auto const &operator*() const{
			return map;
		}

		const auto *operator->() const{
			return &map;
		}

	protected:
		template<class T>
		void register_(T &cmd){
			for(auto &str : T::cmd)
				map[str] = &cmd;
		}

	private:
		using CommandMap = std::unordered_map<std::string_view, cmd_base<Protocol, DBAdapter> *>;

		CommandMap map;
	};



	template<class Protocol, class DBAdapter, bool Mutable>
	struct KeyValueWorkerCommandMap;



	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerCommandMap<Protocol, DBAdapter, false> : KeyValueWorkerCommandMapBase<Protocol, DBAdapter>{
		cmd_EXIT	<Protocol, DBAdapter>	exit		;
		cmd_SHUTDOWN	<Protocol, DBAdapter>	shutdown	;

		cmd_INFO	<Protocol, DBAdapter>	info		;
		cmd_SAVE	<Protocol, DBAdapter>	save		;
		cmd_RELOAD	<Protocol, DBAdapter>	reload		;

		cmd_GET		<Protocol, DBAdapter>	get		;
		cmd_GETX	<Protocol, DBAdapter>	getx		;

		cmd_COUNT	<Protocol, DBAdapter>	count		;
		cmd_SUM		<Protocol, DBAdapter>	sum		;
		cmd_MIN		<Protocol, DBAdapter>	min		;
		cmd_MAX		<Protocol, DBAdapter>	max		;

		KeyValueWorkerCommandMap(){
			auto R = [this](auto &x){
				this->register_(x);
			};

			R(exit		);
			R(shutdown	);

			R(info		);
			R(save		);
			R(reload	);

			R(get		);
			R(getx		);

			R(count		);
			R(sum		);
			R(min		);
			R(max		);
		}
	};



	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerCommandMap<Protocol, DBAdapter, true> : KeyValueWorkerCommandMap<Protocol, DBAdapter, false>{
		cmd_SET		<Protocol, DBAdapter>	set		;
		cmd_SETEX	<Protocol, DBAdapter>	setex		;
		cmd_DEL		<Protocol, DBAdapter>	del		;
		cmd_GETSET	<Protocol, DBAdapter>	getset		;
		cmd_INCR	<Protocol, DBAdapter>	incr		;
		cmd_DECR	<Protocol, DBAdapter>	decr		;

		KeyValueWorkerCommandMap(){
			auto R = [this](auto &x){
				this->register_(x);
			};

			R(set		);
			R(setex		);
			R(del		);
			R(getset	);
			R(incr		);
			R(decr		);
		}
	};



	// ====================================



	template<class Protocol, class DBAdapter>
	struct KeyValueWorker{
		KeyValueWorker(DBAdapter &db) : db_(db){}

		WorkerStatus operator()(IOBuffer &buffer);

	private:
		Protocol	protocol_;
		DBAdapter	&db_;

		KeyValueWorkerCommandMap<Protocol, DBAdapter, DBAdapter::MUTABLE>	map_;
	};



	// ==================================



	template<class Protocol, class DBAdapter>
	WorkerStatus KeyValueWorker<Protocol, DBAdapter>::operator()(IOBuffer &buffer){
		using Status  = protocol::ProtocolStatus;

		// PASS

		if (buffer.size() == 0)
			return WorkerStatus::PASS;

		const Status status = protocol_( std::string_view{ buffer } );

		if (status == Status::BUFFER_NOT_READ)
			return WorkerStatus::PASS;

		// ERROR

		buffer.clear();

		if (status == Status::ERROR)
			return error::InternalError(protocol_, buffer);

		// FETCH command

		const auto &str = protocol_.getParams().front();

		auto it = map_->find(str);

		if (it == std::end(*map_))
			return error::NotImplemented(protocol_, buffer);

		auto &command = *it->second;

		// EXEC command

		return command(protocol_, db_, buffer);
	}



} // namespace



#endif
