#ifndef KEY_VALUE_COMMANDS_H_
#define KEY_VALUE_COMMANDS_H_

#include "mystring.h"

namespace net{
namespace worker{

struct RedisCommands{
	enum class Command : uint8_t{
		UNKNOWN		,

		// System

		EXIT		,
		SHUTDOWN	,

		INFO		,
		SAVE		,
		RELOAD		,

		// Immutable

		GET		,
		GETX		,

		COUNT		,
		SUM		,
		MIN		,
		MAX		,

		// Mutable

		SET		,
		SETEX		,
		DEL		,

		// Higher Level Atomic Counters

		INCR		,
		DECR		,
		GETSET
	};

	constexpr static auto h(const char *s){
		return hash(s);
	}

	constexpr static Command get(std::string_view const cmd){
		// using namespace std::literals;

		switch(hash(cmd)){

		// System

		case h("exit"		)	:
		case h("EXIT"		)	: return Command::EXIT		;

		case h("shutdown"	)	:
		case h("SHUTDOWN"	)	: return Command::SHUTDOWN	;

		case h("info"		)	:
		case h("INFO"		)	: return Command::INFO		;

		case h("save"		)	:
		case h("SAVE"		)	:
		case h("bgsave"		)	:
		case h("BGSAVE"		)	: return Command::SAVE		;

		case h("reload"		)	:
		case h("RELOAD"		)	: return Command::RELOAD	;

		// Immutable

		case h("get"		)	:
		case h("GET"		)	: return Command::GET		;

		case h("getx"		)	:
		case h("GETX"		)	: return Command::GETX		;

		case h("count"		)	:
		case h("COUNT"		)	: return Command::COUNT		;

		case h("sum"		)	:
		case h("SUM"		)	: return Command::SUM		;

		case h("min"		)	:
		case h("MIN"		)	: return Command::MIN		;

		case h("max"		)	:
		case h("MAX"		)	: return Command::MAX		;

		// Mutable

		case h("set"		)	:
		case h("SET"		)	: return Command::SET		;

		case h("setex"		)	:
		case h("SETEX"		)	: return Command::SETEX		;

		case h("del"		)	:
		case h("DEL"		)	: return Command::DEL		;

		// Higher Level Atomic Counters

		case h("incr"		)	:
		case h("INCR"		)	:
		case h("incrby"		)	:
		case h("INCRBY"		)	: return Command::INCR		;

		case h("decr"		)	:
		case h("DECR"		)	:
		case h("decrby"		)	:
		case h("decrBY"		)	: return Command::DECR		;

		case h("getset"		)	:
		case h("GETSET"		)	: return Command::GETSET	;

		// EOF

		default				: return Command::UNKNOWN	;
		}
	}
};

} // namespace worker
} // namespace

#endif

