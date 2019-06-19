
// selector definiton already available here.

#include "sockets.h"
#include "asyncloop.h"

int main(){
	int const fd1 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2000);
	int const fd2 = net::socket_create(net::SOCKET_TCP{},  "localhost.not.used.yet", 2002);
	int const fd3 = net::socket_create(net::SOCKET_UNIX{}, "/tmp/echo");

	size_t const conf_max_clients = 4;

	using MyWorker = MyWorkerFactory::Worker;

	MyWorkerFactory wf;

	net::AsyncLoop<MySelector, MyWorker> loop(
					/* selector */	MySelector{ conf_max_clients },
					/* worker */	wf(),
					/* server fd */	{ fd1, fd2, fd3 },
					conf_max_clients
	);

	while(loop.process());
}

