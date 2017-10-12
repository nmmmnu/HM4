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

	constexpr static Command get(const StringRef &cmd){
		switch(cmd.hash()){
		case "exit"_sr.hash()		:
		case "EXIT"_sr.hash()		: return Command::EXIT		;

		case "shutdown"_sr.hash()	:
		case "SHUTDOWN"_sr.hash()	: return Command::SHUTDOWN	;

		case "info"_sr.hash()		:
		case "INFO"_sr.hash()		: return Command::INFO		;

		case "save"_sr.hash()		:
		case "SAVE"_sr.hash()		:
		case "bgsave"_sr.hash()		:
		case "BGSAVE"_sr.hash()		: return Command::REFRESH	;

		case "get"_sr.hash()		:
		case "GET"_sr.hash()		: return Command::GET		;

		case "hgetall"_sr.hash()	:
		case "HGETALL"_sr.hash()	: return Command::GETALL	;

		case "set"_sr.hash()		:
		case "SET"_sr.hash()		: return Command::SET		;

		case "setex"_sr.hash()			:
		case "SETEX"_sr.hash()		: return Command::SETEX		;

		case "del"_sr.hash()		:
		case "DEL"_sr.hash()		: return Command::DEL		;

		default				: return Command::UNKNOWN	;
		}
	}
};

} // namespace worker
} // namespace

