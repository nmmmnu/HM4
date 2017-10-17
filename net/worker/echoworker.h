#ifndef _ECHO_WORKER_H
#define _ECHO_WORKER_H

#include "workerdefs.h"

#include "stringref.h"

namespace net{
namespace worker{

class EchoWorker{
public:
	template<class CONNECTION>
	WorkerStatus operator()(CONNECTION &buffer);

private:
	static constexpr StringRef cmd_hello	= "hello\r\n";
	static constexpr StringRef cmd_help	= "help\r\n";
	static constexpr StringRef cmd_exit	= "exit\r\n";
	static constexpr StringRef cmd_shutdown	= "shutdown\r\n";

	static constexpr StringRef msg_help	=
				"Usage:\r\n"
				"   hello    - greeting\r\n"
				"   help     - this message\r\n"
				"   exit     - disconnect\r\n"
				"   shutdown - shutdown the server\r\n"
				"\r\n";

private:
	template<class CONNECTION>
	static bool cmp_(const CONNECTION &b, const StringRef &cmd){
		return cmd.compare(b.data(), b.size()) == 0;
	}
};

} // namespace worker
} // namespace

// ==================================

#include "echoworker.h.cc"

#endif
