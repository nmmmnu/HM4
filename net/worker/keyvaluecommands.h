#include "stringref.h"

#include <cstdint>
#include <array>

namespace net{
namespace worker{

struct RedisCommands{
public:
	enum class Command : uint8_t{
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

	struct CmdPair{
		StringRef	str;
		Command		cmd;

		bool operator == (const StringRef &s) const{
			return str == s;
		}
	};

public:
	constexpr static size_t ARRAY_SIZE = 20;

	using CommandsArray = std::array<CmdPair, ARRAY_SIZE>;

	constexpr static CommandsArray commands{
		CmdPair{ "exit",	Command::EXIT		}, CmdPair{ "EXIT",	Command::EXIT		},
		CmdPair{ "shutdown",	Command::SHUTDOWN	}, CmdPair{ "SHUTDOWN",	Command::SHUTDOWN	},

		CmdPair{ "info",	Command::INFO		}, CmdPair{ "INFO",	Command::INFO		},
		CmdPair{ "save",	Command::REFRESH	}, CmdPair{ "SAVE",	Command::REFRESH	},
		CmdPair{ "bgsave",	Command::REFRESH	}, CmdPair{ "BGSAVE",	Command::REFRESH	},

		CmdPair{ "get",		Command::GET		}, CmdPair{ "GET",	Command::GET		},
		CmdPair{ "hgetall",	Command::GETALL		}, CmdPair{ "HGETALL",	Command::GETALL		},

		CmdPair{ "set",		Command::SET		}, CmdPair{ "SET",	Command::SET		},
		CmdPair{ "setex",	Command::SETEX		}, CmdPair{ "SETEX",	Command::SETEX		},

		CmdPair{ "del",		Command::DEL		}, CmdPair{ "DEL",	Command::DEL		}
	};
};


} // namespace worker
} // namespace

