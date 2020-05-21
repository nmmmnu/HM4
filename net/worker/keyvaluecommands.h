#include "mystring.h"

namespace net{
namespace worker{

struct RedisCommands{
public:
	enum class Command : uint8_t{
		UNKNOWN		,

		EXIT		,
		SHUTDOWN	,

		INFO		,
		REFRESH		,

		GET		,
		GETALL		,
		GETX		,

		SET		,
		SETEX		,
		DEL		,

		INCR		,

		COUNT		,
		SUM
	};

	constexpr static auto h(const char *s){
		return hash(s);
	}

	constexpr static Command get(std::string_view const cmd){
		// using namespace std::literals;

		switch(hash(cmd)){
		case h("exit"		)	:
		case h("EXIT"		)	: return Command::EXIT		;

		case h("shutdown"	)	:
		case h("SHUTDOWN"	)	: return Command::SHUTDOWN	;

		case h("info"		)	:
		case h("INFO"		)	: return Command::INFO		;

		case h("save"		)	:
		case h("SAVE"		)	:
		case h("bgsave"		)	:
		case h("BGSAVE"		)	: return Command::REFRESH	;

		case h("get"		)	:
		case h("GET"		)	: return Command::GET		;

		case h("hgetall"	)	:
		case h("HGETALL"	)	: return Command::GETALL	;

		case h("getx"		)	:
		case h("GETX"		)	: return Command::GETX		;

		case h("set"		)	:
		case h("SET"		)	: return Command::SET		;

		case h("setex"		)	:
		case h("SETEX"		)	: return Command::SETEX		;

		case h("del"		)	:
		case h("DEL"		)	: return Command::DEL		;

		case h("incr"		)	:
		case h("INCR"		)	:
		case h("incrby"		)	:
		case h("INCRBY"		)	: return Command::INCR		;

		case h("count"		)	:
		case h("COUNT"		)	: return Command::COUNT		;

		case h("sum"		)	:
		case h("SUM"		)	: return Command::SUM		;

		default				: return Command::UNKNOWN	;
		}
	}
};

} // namespace worker
} // namespace

