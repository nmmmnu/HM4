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
#include "keyvaluecommands.h"



namespace net{
namespace worker{



	template<class Protocol, class DBAdapter, bool Mutable>
	struct KeyValueWorkerExecCommand;



	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerExecCommand<Protocol, DBAdapter, false>{
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

		using Command = RedisCommands::Command;

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, const Command cmd);
	};



	template<class Protocol, class DBAdapter>
	struct KeyValueWorkerExecCommand<Protocol, DBAdapter, true>{
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

		cmd_SET		<Protocol, DBAdapter>	set		;
		cmd_SETEX	<Protocol, DBAdapter>	setex		;
		cmd_DEL		<Protocol, DBAdapter>	del		;
		cmd_GETSET	<Protocol, DBAdapter>	getset		;

		cmd_INCR	<Protocol, DBAdapter>	incr		;
		cmd_DECR	<Protocol, DBAdapter>	decr		;

		using Command = RedisCommands::Command;

		WorkerStatus operator()(Protocol &protocol, DBAdapter &db, IOBuffer &buffer, const Command cmd);
	};



	// ====================================



	template<class Protocol, class DBAdapter>
	class KeyValueWorker{
	public:
		KeyValueWorker(DBAdapter &db) : db_(db){}

		WorkerStatus operator()(IOBuffer &buffer);

	private:
		Protocol	protocol_;
		DBAdapter	&db_;
	};


} // namespace worker
} // namespace


// ==================================

#include "keyvalueworker.h.cc"

#endif
