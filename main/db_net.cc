#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "factory/singlelist.h"
#include "factory/immutable.h"
#include "factory/mutable.h"
#include "factory/mutablebinlog.h"
#include "factory/mutableconcurrent.h"
#include "factory/mutablebinlogcurrent.h"

#include "pmallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"

#include "version.h"
#include "myfs.h"

// ----------------------------------

#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"

// ----------------------------------

#if defined SELECTOR_EPOOL
	#include "selector/epollselector.h"

	using MySelector	= net::selector::EPollSelector;
#elif defined SELECTOR_KQUEUE
	#include "selector/kqueueselector.h"

	using MySelector	= net::selector::KQueueSelector;
#elif defined SELECTOR_POOL
	#include "selector/pollselector.h"

	using MySelector	= net::selector::PollSelector;
#else
	#error "No net::selector selected!"
#endif

constexpr bool USE_CONCURRENCY = true;

using MyProtocol	= net::protocol::RedisProtocol;

using MyArenaAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::ArenaAllocator>;
using MySTDAllocator	= MyAllocator::PMOwnerAllocator<MyAllocator::STDAllocator>;

// ----------------------------------

constexpr size_t MB = 1024 * 1024;

constexpr size_t MIN_ARENA_SIZE = 128;

// ----------------------------------

#include "signalguard.h"
#include "mystring.h"
#include "db_net_options.h"

#include <iostream>

namespace{
	MyOptions prepareOptions(int argc, char **argv);

	template<class FACTORY>
	int main2(const MyOptions &opt, FACTORY &&adapter_factory);


	template<class Factory, typename ...Args>
	int fLists(const MyOptions &opt, Factory &&adapter_factory, Args &&... args){
		fmt::print(std::clog, std::forward<Args>(args)...);
		return main2(opt, std::move(adapter_factory));
	}

	int immutableLists(const MyOptions &opt){
		using hm4::listloader::DirectoryListLoader;

		if (DirectoryListLoader::checkIfLoaderNeed(opt.db_path))
			return fLists(
				opt, DBAdapterFactory::Immutable{ opt.db_path },
				"Starting immutable server...\n"
			);
		else
			return fLists(
				opt, DBAdapterFactory::SingleList{ opt.db_path },
				"Starting singlelist server...\n"
			);
	}

	template<class Allocator>
	Allocator createAllocator(const MyOptions &opt);

	template<>
	MySTDAllocator createAllocator(const MyOptions &){
		return MySTDAllocator{};
	}

	template<>
	MyArenaAllocator createAllocator(const MyOptions &opt){
		size_t const max_memlist_arena = opt.max_memlist_arena < MIN_ARENA_SIZE ? MIN_ARENA_SIZE : opt.max_memlist_arena * MB;

		return MyArenaAllocator{ max_memlist_arena };
	}

	template<class Allocator>
	int mutableLists_binlog(const MyOptions &opt){
		size_t const max_memlist_size  = opt.max_memlist_size  * MB;

		if constexpr(USE_CONCURRENCY){
			bool const have_binlog = ! opt.binlog_path1.empty() && ! opt.binlog_path2.empty();

			auto allocator1 = createAllocator<Allocator>(opt);
			auto allocator2 = createAllocator<Allocator>(opt);

			auto allocatorName = allocator1.getName();

			if (have_binlog){
				return fLists(
					opt, DBAdapterFactory::MutableBinLogConcurrent{ opt.db_path, opt.binlog_path1, opt.binlog_path2, opt.binlog_fsync != 0, max_memlist_size, allocator1, allocator2 },
					"Starting {} server with {}...\n", "mutable concurrent binlog", allocatorName
				);
			}else{
				return fLists(
					opt, DBAdapterFactory::MutableConcurrent{   opt.db_path, max_memlist_size, allocator1, allocator2 },
					"Starting {} server with {}...\n", "mutable concurrent", allocatorName
				);
			}
		}else{
			bool const have_binlog = ! opt.binlog_path1.empty();

			auto allocator = createAllocator<Allocator>(opt);

			auto allocatorName = allocator.getName();

			if (have_binlog){
				return fLists(
					opt, DBAdapterFactory::MutableBinLog{ opt.db_path, opt.binlog_path1, opt.binlog_fsync != 0, max_memlist_size, allocator },
					"Starting {} server with {}...\n", "mutable binlog", allocatorName
				);
			}else{
				return fLists(
					opt, DBAdapterFactory::Mutable{   opt.db_path, max_memlist_size, allocator },
					"Starting {} server with {}...\n", "mutable", allocatorName
				);
			}
		}
	}

	int mutableLists_allocator(const MyOptions &opt){
		if (opt.max_memlist_arena)
			return mutableLists_binlog<MyArenaAllocator	>(opt);
		else
			return mutableLists_binlog<MySTDAllocator	>(opt);
	}

	int mutableLists(const MyOptions &opt){
		return mutableLists_allocator(opt);
	}
}

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	if (opt.immutable)
		return immutableLists(opt);
	else
		return mutableLists(opt);
}

namespace{

	void printUsage(const char *cmd);
	void printError(const char *msg);

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
			opt.port	= from_string<uint16_t>(argv[3]);

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

		if (!opt.binlog_path1.empty() && fileExists(hm4::disk::filenameData(opt.binlog_path1)))
			printError("Binlog file exists. please replay and remove it...");

		return opt;
	}

	template<class Factory>
	int main2(const MyOptions &opt, Factory &&adapter_factory){
		using MyAdapterFactory	= Factory;
		using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
		using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
		using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

		size_t const max_packet_size = hm4::Pair::maxBytes() * 2;

		uint16_t const socket_options = opt.tcp_reuseport ?
				net::SOCKET_DEFAULTOPT_TCP	:
				net::SOCKET_DEFAULTOPT_TCP | net::SOCKET_REUSEPORT
		;

		int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port,  socket_options);

		if (fd < 0)
			printError("Can not create server socket...");

		MyLoop loop{
				/* selector */	MySelector	{ opt.max_clients },
				/* worker */	MyWorker	{ adapter_factory() },
				/* server fd */	{ fd },
				opt.max_clients, opt.timeout, max_packet_size
		};

		SignalGuard guard;

		auto signal_processing = [&adapter_factory](Signal const signal){
			switch(signal){
			case Signal::HUP:
				adapter_factory().save();
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
		fmt::print(stderr, "{}\n", msg);
		exit(1);
	}

	void printUsage(const char *cmd){
		#ifdef NOT_HAVE_CHARCONV
		const char *convert = "sstream";
		#else
		const char *convert = "charconv";
		#endif

		fmt::print(
			"db_net version {version}\n"
			"\n"
			"Build:\n"
			"\tSelector"	"\t{selector}\n"
			"\tConvertion"	"\t{convert}\n"
			"\n"
			"Usage:\n"
			"\t{cmd} [configuration file] - start server\n"
			"\t...or...\n"
			"\t{cmd} r [lsm_path] [optional tcp port] - start immutable  server\n"
			"\t{cmd} w [lsm_path] [optional tcp port] - start mutable    server\n"
			"\n"
			"\t\tPath names must be written with quotes:\n"
			"\t\tExample directory/file.'*'.db\n"
			"\t\tThe '*', will be replaced with ID's\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("selector",	MySelector::NAME	),
			fmt::arg("convert",	convert			),
			fmt::arg("cmd",		cmd			)
		);

		fmt::print("INI File Usage:\n");

		MyOptions::print();

		fmt::print("\n");

		exit(10);
	}

} // anonymous namespace


