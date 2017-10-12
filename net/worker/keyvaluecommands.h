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
		case "exit"_srh		:
		case "EXIT"_srh		: return Command::EXIT		;

		case "shutdown"_srh	:
		case "SHUTDOWN"_srh	: return Command::SHUTDOWN	;

		case "info"_srh		:
		case "INFO"_srh		: return Command::INFO		;

		case "save"_srh		:
		case "SAVE"_srh		:
		case "bgsave"_srh	:
		case "BGSAVE"_srh	: return Command::REFRESH	;

		case "get"_srh		:
		case "GET"_srh		: return Command::GET		;

		case "hgetall"_srh	:
		case "HGETALL"_srh	: return Command::GETALL	;

		case "set"_srh		:
		case "SET"_srh		: return Command::SET		;

		case "setex"_srh	:
		case "SETEX"_srh	: return Command::SETEX		;

		case "del"_srh		:
		case "DEL"_srh		: return Command::DEL		;

		default			: return Command::UNKNOWN	;
		}
	}
};

} // namespace worker
} // namespace

