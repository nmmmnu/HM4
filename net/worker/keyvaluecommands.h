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
	constexpr static size_t BUCKETS		= 133;

	using Map = StringMap<Command, BUCKETS, Command::UNKNOWN>;

public:
	constexpr static Map commands = {
		{ "exit",	Command::EXIT		}, { "EXIT",	Command::EXIT		},
		{ "shutdown",	Command::SHUTDOWN	}, { "SHUTDOWN", Command::SHUTDOWN	},

		{ "info",	Command::INFO		}, { "INFO",	Command::INFO		},
		{ "save",	Command::REFRESH	}, { "SAVE",	Command::REFRESH	},
		{ "bgsave",	Command::REFRESH	}, { "BGSAVE",	Command::REFRESH	},

		{ "get",	Command::GET		}, { "GET",	Command::GET		},
		{ "hgetall",	Command::GETALL		}, { "HGETALL",	Command::GETALL		},

		{ "set",	Command::SET		}, { "SET",	Command::SET		},
		{ "setex",	Command::SETEX		}, { "SETEX",	Command::SETEX		},

		{ "del",	Command::DEL		}, { "DEL",	Command::DEL		}
	};

	static_assert( commands, "Collision, change number of buckets to something else" );
};


} // namespace worker
} // namespace

