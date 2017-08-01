#ifndef _KEY_VALUE_WORKER_H
#define _KEY_VALUE_WORKER_H

#include "workerdefs.h"

#include "stringref.h"

#include "iobuffer.h"

#include "stringref.h"

#include <vector>

#include <array>

namespace net{
namespace worker{

struct KeyValueWorkerStrings{
public:
	template <size_t N>
	using Strings	= std::array<StringRef, N>;

public:
	static constexpr Strings<2> EXIT	{{	"EXIT",		"exit"		}};
	static constexpr Strings<2> SHUTDOWN	{{	"SHUTDOWN",	"shutdown"	}};
	static constexpr Strings<2> INFO	{{	"INFO",		"info"		}};
	static constexpr Strings<4> REFRESH	{{	"SAVE",		"save",
							"BGSAVE",	"bgsave"	}};
	static constexpr Strings<2> GET		{{	"GET",		"get"		}};
	static constexpr Strings<2> GETALL	{{	"HGETALL",	"hgetall"	}};

public:
	static constexpr bool cmp__(const StringRef &cmd, const Strings<4> &m){
		return cmd == m[0] || cmd == m[1] || cmd == m[2] || cmd == m[3];
	}

	static constexpr bool cmp__(const StringRef &cmd, const Strings<2> &m){
		return cmd == m[0] || cmd == m[1];
	}
};

template<class PROTOCOL, class DB_ADAPTER>
class KeyValueWorker : private KeyValueWorkerStrings{
public:
	KeyValueWorker(DB_ADAPTER &db) : db_(db){}

	template<class CONNECTION>
	WorkerStatus operator()(CONNECTION &buffer);

private:
	template<class CONNECTION>
	WorkerStatus process_request_(CONNECTION &buffer);

	enum class ResponseType{
		ERROR,
		STRING,
		MULTI
	};

	template<class CONNECTION>
	WorkerStatus sendResponseError_(CONNECTION &buffer, const StringRef &msg);

	template<class CONNECTION>
	WorkerStatus sendResponseString_(CONNECTION &buffer, const StringRef &msg);

	template<class CONNECTION, class CONTAINER>
	WorkerStatus sendResponseMulti_(CONNECTION &buffer, const CONTAINER &msg);

	template<class CONNECTION>
	WorkerStatus sendResponseBadRequest_(CONNECTION &buffer){
		return sendResponseError_(buffer, "Bad request");
	}

private:
	PROTOCOL	protocol_;
	DB_ADAPTER	&db_;
};


} // namespace worker
} // namespace


// ==================================

#include "keyvalueworker_impl.h"

#endif
