#include "db_net_immutablefactory.h"
#include "db_net_mutablefactory.h"
#include "db_net_singlelistfactory.h"

#include "version.h"

// ----------------------------------

#include "selector/epollselector.h"
#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"

// ----------------------------------

using MySelector	= net::selector::EPollSelector;
using MyProtocol	= net::protocol::RedisProtocol;

// ----------------------------------

size_t constexpr MB = 1024 * 1024;

// ----------------------------------

#include "signalguard.h"
#include "ston_safe.h"
#include "db_net_options.h"

#include <iostream>

namespace{
	MyOptions prepareOptions(int argc, char **argv);

	template<class FACTORY>
	int main2(const MyOptions &opt, FACTORY &&adapter_f);

	void printUsage(const char *cmd);
	void printError(const char *msg);
}

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	size_t const max_memlist_size = opt.max_memlist_size * MB;

	using hm4::listloader::DirectoryListLoader;

	if (opt.immutable == 0){
		std::clog << "Starting mutable server..."	<< '\n';
		return main2(opt, MyMutableDBAdapterFactory{   opt.db_path, max_memlist_size } );

	}else if (DirectoryListLoader::checkIfLoaderNeed(opt.db_path)){
		std::clog << "Starting immutable server..."	<< '\n';
		return main2(opt, MyImmutableDBAdapterFactory{ opt.db_path, max_memlist_size } );

	}else{
		std::clog << "Starting singlelist server..."	<< '\n';
		return main2(opt, MySingleListDBAdapterFactory{ opt.db_path, max_memlist_size } );
	}
}

namespace{

	MyOptions prepareOptions(int argc, char **argv){
		auto argImutable = [](const char *s) -> uint16_t{
			switch(s[0]){
			default:
			case 'r': return 1;
			case 'w': return 0;
			}
		};

		MyOptions opt;

		switch(argc){
		case 1 + 1:
			if (! readINIFile(argv[1], opt))
				printError("Can not open config file...");

			break;

		case 3 + 1:
			opt.port	= ston_safe<uint16_t>(argv[3]);

			opt.immutable	= argImutable(argv[1]);
			opt.db_path	= argv[2];
			break;

		case 2 + 1:
			opt.immutable	= argImutable(argv[1]);
			opt.db_path	= argv[2];
			break;

		default:
			printUsage(argv[0]);
		}

		if (opt.port == 0)
			printError("Can not create server socket on port zero...");

		return opt;
	}

	template<class FACTORY>
	int main2(const MyOptions &opt, FACTORY &&adapter_factory){
		using MyAdapterFactory	= FACTORY;
		using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
		using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
		using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

		size_t const max_packet_size = hm4::Pair::maxBytes() * 2;

		int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port);

		if (fd < 0)
			printError("Can not create server socket...");

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

	void printError(const char *msg){
		std::cout << msg << '\n';
		exit(1);
	}

	void printUsage(const char *cmd){
		std::cout
			<< "db_net version " << hm4::version::str 							<< '\n'
					<< '\n'
			<< "Usage:"	<< '\n'
			<< "\t"		<< cmd	<< " [configuration file] - start server"				<< '\n'
			<< "...or..."	<< '\n'
			<< "\t"		<< cmd	<< " r [lsm_path] [optional tcp port] - start immutable  server"	<< '\n'
			<< "\t"		<< cmd	<< " w [lsm_path] [optional tcp port] - start mutable    server"	<< '\n'

			<< "\t\tPath names must be written with quotes:"	<< '\n'
			<< "\t\tExample directory/file.'*'.db"			<< '\n'
			<< "\t\tThe '*', will be replaced with ID's"		<< '\n'

			<< '\n';

		std::cout
			<< "INI File Usage:"	<< '\n';

		MyOptions::print();

		std::cout
			<< '\n';

		exit(10);
	}

} // anonymous namespace


