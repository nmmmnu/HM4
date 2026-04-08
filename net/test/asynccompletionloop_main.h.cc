
// selector definiton already available here.

#include "sockets.h"
#include "asynccompletionloop.h"

int main(){
	int const fd1 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2000);
	int const fd2 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2002);
	int const fd3 = net::socket_create(net::SOCKET_UNIX{}, "/tmp/echo");

	uint32_t const conf_rlimitNoFile = static_cast<uint32_t>(net::socket_get_rlimit_nofile());
	size_t   const conf_max_clients  = 32;

	using MyWorker = MyWorkerFactory::Worker;

	MyWorkerFactory wf;

	net::AsyncCompletionLoop<MySelector, MyWorker> loop(
					/* selector */	MySelector{ conf_rlimitNoFile, conf_max_clients, 5, 2 * 60, 2 * 60 },
					/* worker */	wf()			,
					/* server fd */	{ fd1, fd2, fd3 }	,
					conf_rlimitNoFile			,
					conf_max_clients
	);

	while(loop.process());
}

