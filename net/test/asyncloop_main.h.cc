
// selector definiton already available here.

#include "sockets.h"
#include "asyncloop.h"
#include "simplememorymanager.h"

int main(){
	int const fd1 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2000);
	int const fd2 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2002);
	int const fd3 = net::socket_create(net::SOCKET_UNIX{}, "/tmp/echo");

	uint32_t const conf_rlimitNoFile = static_cast<uint32_t>(net::socket_get_rlimit_nofile());
	size_t   const conf_max_clients  = 6;

	using MyWorker		= MyWorkerFactory::Worker;
	using MyMemoryManager	= memory_manager::SimpleMemoryManager;

	MyWorkerFactory wf;

	MyMemoryManager mm;

	net::AsyncLoop<MySelector, MyWorker, MyMemoryManager> loop(
					/* selector */	MySelector{ conf_rlimitNoFile, conf_max_clients },
					/* worker */	wf()			,
					/* server fd */	{ fd1, fd2, fd3 }	,
					mm					,
					conf_rlimitNoFile			,
					conf_max_clients
	);

	while(loop.process());
}

