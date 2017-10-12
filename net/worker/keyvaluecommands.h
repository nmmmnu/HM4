#include "stringmap.h"

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

		SET		,
		SETEX		,
		DEL
	};

private:
	constexpr static auto h_(const StringRef &name){
		return name.hash();
	}

public:
	constexpr static Command get(const StringRef &cmd){
		switch(cmd.hash()){
		case h_("exit"		)	:
		case h_("EXIT"		)	: return Command::EXIT		;

		case h_("shutdown"	)	:
		case h_("SHUTDOWN"	)	: return Command::SHUTDOWN	;

		case h_("info"		)	:
		case h_("INFO"		)	: return Command::INFO		;

		case h_("save"		)	:
		case h_("SAVE"		)	:
		case h_("bgsave"	)	:
		case h_("BGSAVE"	)	: return Command::REFRESH	;

		case h_("get"		)	:
		case h_("GET"		)	: return Command::GET		;

		case h_("hgetall"	)	:
		case h_("HGETALL"	)	: return Command::GETALL	;

		case h_("set"		)	:
		case h_("SET"		)	: return Command::SET		;

		case h_("setex"		)	:
		case h_("SETEX"		)	: return Command::SETEX		;

		case h_("del"		)	:
		case h_("DEL"		)	: return Command::DEL		;

		default				: return Command::UNKNOWN	;
		}
	}
};

} // namespace worker
} // namespace

