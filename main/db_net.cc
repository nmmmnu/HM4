#include "db_net_immutablefactory.h"
#include "db_net_mutablefactory.h"

// ----------------------------------

#include "selector/pollselector.h"
#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"

// ----------------------------------

using MySelector	= net::selector::PollSelector;
using MyProtocol	= net::protocol::RedisProtocol;

// ----------------------------------

static int printUsage(const char *cmd);
static int printError(const char *msg);

// ----------------------------------

size_t constexpr MB = 1024 * 1024;

// ----------------------------------

#include "signalguard.h"
#include "stou.h"
#include "inifile.h"
#include "db_net_options.h"

template<class FACTORY>
static int main2(const MyOptions &opt, FACTORY &&adapter_f);

int main(int argc, char **argv){
	MyOptions opt;

	switch(argc){
	case 1 + 1:
		readINIFile(argv[1], opt);
		break;

	case 3 + 1:
		opt.port	= stou<uint16_t>(argv[3]);
		/* continue */

	case 2 + 1:
		opt.immutable	= argv[1][0] == 'r';
		opt.db_path	= argv[2];
		break;

	default:
		return printUsage(argv[0]);
	}

	if (opt.port == 0)
		return printError("Can not create server socket on invalid port.");

	// ----------------------------------

	size_t const max_memlist_size = opt.max_memlist_size * MB;

	if (opt.immutable)
		return main2(opt, MyImmutableDBAdapterFactory{ opt.db_path, max_memlist_size } );
	else
		return main2(opt, MyMutableDBAdapterFactory{   opt.db_path, max_memlist_size } );
}

template<class FACTORY>
static int main2(const MyOptions &opt, FACTORY &&adapter_factory){
	using MyAdapterFactory	= FACTORY;
	using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
	using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
	using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

	size_t const max_packet_size = hm4::Pair::maxBytes() * 2;

	int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port);

	if (fd < 0)
		return printError("Can not create server socket.");

	MyLoop loop( MySelector{ opt.max_clients }, MyWorker{ adapter_factory() }, { fd },
							opt.timeout, max_packet_size);

	SignalGuard guard;

	while(loop.process() && guard);

	return 0;
}

// ----------------------------------

#include <iostream>

static int printError(const char *msg){
	std::cout << msg << '\n';
	return 1;
}

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< "[configuration file] - start server"			<< '\n'
		<< "...or..."	<< '\n'
		<< "\t"		<< cmd	<< " r [lsm_path] [optional tcp port] - start immutable server"	<< '\n'
		<< "\t"		<< cmd	<< " w [lsm_path] [optional tcp port] - start mutable   server"	<< '\n'

		<< "\t\tPath names must be written like this:"	<< '\n'
		<< "\t\tExample 'directory/file.*.db'"		<< '\n'

		<< '\n';

	std::cout
		<< "INI File Usage:"	<< '\n';

	MyOptions::print();

	std::cout
		<< '\n';

	return 10;
}

