#include "db_net_immutablefactory.h"
#include "db_net_mutablefactory.h"
#include "db_net_singlelistfactory.h"

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

	auto argImutable = [](const char *s) -> uint16_t{
		switch(s[0]){
		default:
		case 'w': return 0;
		case 'r': return 1;
		case 's': return 2;
		}
	};

	switch(argc){
	case 1 + 1:
		if (! readINIFile(argv[1], opt))
			return printError("Can not open config file...");

		break;

	case 3 + 1:
		opt.port	= stou<uint16_t>(argv[3]);

		opt.immutable	= argImutable(argv[1]);
		opt.db_path	= argv[2];
		break;

	case 2 + 1:
		opt.immutable	= argImutable(argv[1]);
		opt.db_path	= argv[2];
		break;

	default:
		return printUsage(argv[0]);
	}

	if (opt.port == 0)
		return printError("Can not create server socket on port zero...");

	// ----------------------------------

	size_t const max_memlist_size = opt.max_memlist_size * MB;

	switch(opt.immutable){
	default:
	case 0:
		std::clog << "Starting mutable server..."	<< '\n';
		return main2(opt, MyMutableDBAdapterFactory{   opt.db_path, max_memlist_size } );
	case 1:
		std::clog << "Starting immutable server..."	<< '\n';
		return main2(opt, MyImmutableDBAdapterFactory{ opt.db_path, max_memlist_size } );
	case 2:
		std::clog << "Starting singlelist server..."	<< '\n';
		return main2(opt, MySingleListDBAdapterFactory{ opt.db_path, max_memlist_size } );
	}
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
		return printError("Can not create server socket...");

	MyLoop loop( MySelector{ opt.max_clients }, MyWorker{ adapter_factory() }, { fd },
							opt.timeout, max_packet_size);

	SignalGuard guard;

	auto signal_processing = [&adapter_factory](Signal const signal){
		switch(signal){
		case Signal::HUP:
			adapter_factory().refresh(true);
			return true;

		case Signal::INT:
		case Signal::TERM:
			return false;

		default:
			return true;
		}
	};

	while( loop.process() && signal_processing(guard()) );

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
		<< "\t"		<< cmd	<< " [configuration file] - start server"			<< '\n'
		<< "...or..."	<< '\n'
		<< "\t"		<< cmd	<< " r [lsm_path] [optional tcp port] - start immutable  server"	<< '\n'
		<< "\t"		<< cmd	<< " w [lsm_path] [optional tcp port] - start mutable    server"	<< '\n'
		<< "\t"		<< cmd	<< " s [lsm_path] [optional tcp port] - start singlelist server"	<< '\n'

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

