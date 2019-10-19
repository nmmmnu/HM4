#ifndef _ECHO_WORKER_H
#define _ECHO_WORKER_H

#include "workerdefs.h"

#include "mystring.h"

#include <string_view>

namespace net{
namespace worker{

class EchoWorker{
public:
	template<class CONNECTION>
	WorkerStatus operator()(CONNECTION &buffer);

private:
	constexpr static std::string_view cmd_hello	= "hello"	"\r\n";
	constexpr static std::string_view cmd_help	= "help"	"\r\n";
	constexpr static std::string_view cmd_exit	= "exit"	"\r\n";
	constexpr static std::string_view cmd_shutdown	= "shutdown"	"\r\n";

	constexpr static std::string_view msg_help	=
				"Usage:\r\n"
				"   hello    - greeting\r\n"
				"   help     - this message\r\n"
				"   exit     - disconnect\r\n"
				"   shutdown - shutdown the server\r\n"
				"\r\n";

private:
	template<class CONNECTION>
	static bool cmp_(const CONNECTION &b, std::string_view const cmd){
		return equals(b.data(), b.size(), cmd.data(), cmd.size());
	}
};

} // namespace worker
} // namespace

// ==================================

#include "echoworker.h.cc"

#endif
