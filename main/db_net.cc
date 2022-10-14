#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "factory/singlelist.h"
#include "factory/immutable.h"
#include "factory/mutable.h"
#include "factory/mutablebinlog.h"
#include "factory/mutableconcurrent.h"
#include "factory/mutablebinlogcurrent.h"
#include "factory/binlogreplay.h"

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

using MyArenaAllocator	= MyAllocator::ArenaAllocator;

// ----------------------------------

constexpr size_t MIN_ARENA_SIZE = 128;

// ----------------------------------

#include "signalguard.h"
#include "mystring.h"
#include "db_net_options.h"

#include <iostream>

namespace{

	void printError(const char *msg){
		fmt::print(stderr, "{}\n", msg);
		exit(1);
	}

	MyOptions prepareOptions(int argc, char **argv);

	template<class Factory>
	int main2(const MyOptions &opt, Factory &&adapter_factory);


	template<class Factory, typename ...Args>
	int fLists(const MyOptions &opt, Factory &&adapter_factory, Args &&... args){
		fmt::print(std::clog, std::forward<Args>(args)...);
		return main2(opt, std::move(adapter_factory));
	}

	int select_ImmutableLists(const MyOptions &opt){
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

	MyArenaAllocator createAllocator(const MyOptions &opt){
		constexpr size_t MB = 1024 * 1024;

		// uncomment for virtual Allocator
		static_assert(MyArenaAllocator::knownMemoryUsage(), "Allocator must know its memory usage");

		if (!MyArenaAllocator::knownMemoryUsage())
			printError("Allocator must know its memory usage");

		auto const max_memlist_arena = std::max(MIN_ARENA_SIZE, opt.max_memlist_arena);

		MyArenaAllocator allocator{ max_memlist_arena * MB };

		fmt::print(std::clog, "Creating {} with size of {} MB\n", allocator.getName(), max_memlist_arena);

		return allocator;
	}

	void replayBinlogFile(std::string_view file, std::string_view path, MyAllocator::ArenaAllocator &allocator);

	void checkBinLogFile(std::string_view file, std::string_view path, MyAllocator::ArenaAllocator &allocator){
		if (file.empty())
			return;

		if (!fileExists(hm4::disk::filenameData(file)))
			return;

		return replayBinlogFile(file, path, allocator);
	}

	int select_MutableLists(const MyOptions &opt){
		if constexpr(USE_CONCURRENCY){
			bool const have_binlog = ! opt.binlog_path1.empty() && ! opt.binlog_path2.empty();

			auto allocator1 = createAllocator(opt);
			auto allocator2 = createAllocator(opt);

			auto allocatorName = allocator1.getName();

			if (have_binlog){
				using SyncOptions = hm4::binlogger::DiskFileBinLogger::SyncOptions;
				SyncOptions syncOprions = opt.binlog_fsync ? SyncOptions::FSYNC : SyncOptions::NONE;

				auto &allocator = allocator1;

				checkBinLogFile(opt.binlog_path1, opt.db_path, allocator);
				checkBinLogFile(opt.binlog_path2, opt.db_path, allocator);

				return fLists(
					opt, DBAdapterFactory::MutableBinLogConcurrent<MyArenaAllocator>{ opt.db_path, opt.binlog_path1, opt.binlog_path2, syncOprions, allocator1, allocator2 },
					"Starting {} server with {}...\n", "mutable concurrent binlog", allocatorName
				);
			}else{
				return fLists(
					opt, DBAdapterFactory::MutableConcurrent<MyArenaAllocator>{   opt.db_path, allocator1, allocator2 },
					"Starting {} server with {}...\n", "mutable concurrent", allocatorName
				);
			}
		}else{
			bool const have_binlog = ! opt.binlog_path1.empty();

			auto allocator = createAllocator(opt);

			auto allocatorName = allocator.getName();

			if (have_binlog){
				using SyncOptions = hm4::binlogger::DiskFileBinLogger::SyncOptions;
				SyncOptions syncOprions = opt.binlog_fsync ? SyncOptions::FSYNC : SyncOptions::NONE;

				checkBinLogFile(opt.binlog_path1, opt.db_path, allocator);

				return fLists(
					opt, DBAdapterFactory::MutableBinLog<MyArenaAllocator>{ opt.db_path, opt.binlog_path1, syncOprions, allocator },
					"Starting {} server with {}...\n", "mutable binlog", allocatorName
				);
			}else{
				return fLists(
					opt, DBAdapterFactory::Mutable<MyArenaAllocator>{   opt.db_path, allocator },
					"Starting {} server with {}...\n", "mutable", allocatorName
				);
			}
		}
	}

	int select_List(const MyOptions &opt){
		if (opt.immutable)
			return select_ImmutableLists(opt);
		else
			return select_MutableLists(opt);
	}

} // anonymous namespace

int main(int argc, char **argv){
	MyOptions const opt = prepareOptions(argc, argv);

	return select_List(opt);
}

namespace{

	void printUsage(const char *cmd);

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

		return opt;
	}

	template<class Factory>
	int main2(const MyOptions &opt, Factory &&adapter_factory){
		using MyAdapterFactory	= Factory;
		using MyDBAdapter	= typename MyAdapterFactory::MyDBAdapter;
		using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
		using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

		if (opt.port == 0)
			printError("Can not create server socket on port zero...");

		size_t const max_packet_size = hm4::Pair::maxBytes() * 2;

		net::options_type const socket_options = opt.tcp_reuseport ?
				net::SOCKET_DEFAULTOPT_TCP | net::SOCKET_REUSEPORT :
				net::SOCKET_DEFAULTOPT_TCP
		;

		int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port, opt.tcp_backlog, socket_options);

		if (fd < 0)
			printError("Can not create server socket...");

		if (opt.tcp_reuseport)
			fmt::print(std::clog, "Warning: Server start with SO_REUSEPORT.\n");

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

			case Signal::USR1:
				adapter_factory().reload();
				return true;

			case Signal::INT:
			case Signal::TERM:
				return false;

			default:
				return true;
			}
		};

		MyTimer timer_reload;

		auto const crontab_reload = opt.crontab_reload == 0 ? 0 : std::min(opt.crontab_reload, opt.crontab_reload_min);

		while( loop.process() && signal_processing(guard()) ){
			crontab(timer_reload, crontab_reload, [&adapter_factory](){
				adapter_factory().reload();
			});
		}

		return 0;
	}

	// ----------------------------------

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
			"\t\t\tExample directory/file.'*'.db\n"
			"\t\t\tThe '*', will be replaced with ID's\n"
			"\t\tYou may overcommit memlist arena, if your system supports it.\n"
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



	void replayBinlogFile_(std::string_view file, std::string_view path, MyAllocator::ArenaAllocator &allocator){

		fmt::print(std::clog, "Binlog file exists. Trying to replay...\n");

		using hm4::disk::DiskList;

		DiskList input;

		auto const result = input.openDataOnly(file, hm4::Pair::WriteOptions::ALIGNED);

		if (! result){
			fmt::print(std::clog, "Replay failed.\n");
			return;
		}

		/* nested scope for the d-tor */
		{
			DBAdapterFactory::BinLogReplay factory{ path, allocator };

			auto &list = factory();

			// no need to show messages.
			// messages will be shown from the builder.
			for(auto const &pair : input)
				if (!pair.empty())
					insert(list, pair);

		} /* d-tor of list kicks here */

		fmt::print(std::clog, "Replay done.\n");
	}

	void replayBinlogFile(std::string_view file, std::string_view path, MyAllocator::ArenaAllocator &allocator){
		replayBinlogFile_(file, path, allocator);
		allocator.reset();
	}

} // anonymous namespace

