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
#include "mmapallocator.h"
#include "allocatedbuffer.h"

#include "version.h"
#include "myfs.h"

// ----------------------------------

#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"
#include "simplesparepool.h"
#include "heapsparepool.h"

// ----------------------------------

#define MEMLIST_AVL
//#define MEMLIST_SKIP

#define REPLAYLIST_AVL
//#define REPLAYLIST_SKIP
//#define REPLAYLIST_UNSORTED

constexpr bool USE_CONCURRENCY = true;

using MyProtocol	= net::protocol::RedisProtocol;

using ArenaBuffer	= MyBuffer::AllocatedByteBufferOwned<MyAllocator::MMapAllocator>;
using Allocator		= MyAllocator::ArenaAllocator;

// ----------------------------------

#if defined MEMLIST_AVL
	#include "avllist.h"

	using MyDBNetMemList = hm4::AVLList<Allocator>;
#elif defined MEMLIST_SKIP
	#include "skiplist.h"

	using MyDBNetMemList = hm4::SkipList<Allocator>;
#else
	#error "No net::memlist selected!"
#endif

// ----------------------------------

#if defined REPLAYLIST_AVL
	#include "avllist.h"

	using MyReplayList = hm4::AVLList<Allocator>;
#elif defined MEMLIST_SKIP
	#include "skiplist.h"

	using MyReplayList = hm4::SkipList<Allocator>;
#elif defined MEMLIST_UNSORTED
	#include "unsortedlist.h"

	using MyReplayList = hm4::UnsortedList<Allocator>;
#else
	#error "No net::replaylist selected!"
#endif

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

// ----------------------------------

constexpr size_t MIN_ARENA_SIZE_BYTES	= 1024 + hm4::Pair::maxBytes() +
					hm4::config::MAX_INTERNAL_NODE_SIZE;

constexpr size_t MIN_ARENA_SIZE		= MIN_ARENA_SIZE_BYTES / 1024 / 1024;

// ----------------------------------

#include "signalguard.h"
#include "mystring.h"
#include "db_net_options.h"

namespace{

	template<typename T, T factor = 1024>
	constexpr T round_up(T n){
		auto const c = n + factor - 1;
		return c - c % factor;
	}

	void printError(const char *msg){
		logger<Logger::FATAL>() << msg;
		exit(1);
	}

	MyOptions prepareOptions(int argc, char **argv);

	template<class Factory>
	int main2(const MyOptions &opt, Factory &&adapter_factory);

	template<class Factory, typename ...Args>
	int fLists(const MyOptions &opt, Factory &&adapter_factory, Args &&...args){
		logger_fmt<Logger::STARTUP>(args...);

		return main2(opt, std::move(adapter_factory));
	}

	int select_ImmutableLists(const MyOptions &opt){
		using hm4::listloader::DirectoryListLoader;

		if (DirectoryListLoader::checkIfLoaderNeed(opt.db_path)){
			return fLists(opt, DBAdapterFactory::Immutable{ opt.db_path },
					"Starting {} server...", "immutable"
			);
		}else{
			return fLists(opt, DBAdapterFactory::SingleList{ opt.db_path },
					"Starting {} server...", "singlelist"
			);
		}
	}

	size_t calcAllocatorSize(const MyOptions &opt){
		constexpr size_t MB = 1024 * 1024;

		return std::max(MIN_ARENA_SIZE, opt.max_memlist_arena) * MB;
	}

	Allocator createAllocator(const MyOptions &opt, ArenaBuffer &buffer){
		// uncomment for virtual Allocator
		static_assert(Allocator::knownMemoryUsage(), "Allocator must know its memory usage");

		Allocator allocator{ buffer };

		logger_fmt<Logger::NOTICE>("{} creating with size of {} bytes", allocator.getName(), buffer.size());

		if (opt.map_memlist_arena){
			logger_fmt<Logger::NOTICE>("{} mapping virtual memory pages to physical/swap memory (may take a while)", allocator.getName());
			allocator.mapPages();
		}

		return allocator;
	}

	void replayBinlogFile(std::string_view file, std::string_view path, Allocator &allocator);

	void checkBinLogFile(std::string_view file, std::string_view path, Allocator &allocator){
		if (file.empty())
			return;

		if (!fileExists(hm4::disk::filenameData(file)))
			return;

		return replayBinlogFile(file, path, allocator);
	}

	int select_MutableLists(const MyOptions &opt){
		constexpr std::string_view starting_server_with = "Starting {} server with {} and {}...";

		using MyMemList = MyDBNetMemList;

		auto const allocatorSize = calcAllocatorSize(opt);

		if constexpr(USE_CONCURRENCY){
			ArenaBuffer buffer1{ allocatorSize };
			ArenaBuffer buffer2{ allocatorSize };

			auto allocator1 = createAllocator(opt, buffer1);
			auto allocator2 = createAllocator(opt, buffer2);

			bool const have_binlog = ! opt.binlog_path1.empty() && ! opt.binlog_path2.empty();

			if (have_binlog){
				using SyncOptions = hm4::binlogger::DiskFileBinLogger::SyncOptions;
				SyncOptions syncOprions = opt.binlog_fsync ? SyncOptions::FSYNC : SyncOptions::NONE;

				// can be done in parallel,
				// but then it will make preasure to the disk

				checkBinLogFile(opt.binlog_path1, opt.db_path, allocator1);
				checkBinLogFile(opt.binlog_path2, opt.db_path, allocator2);

				using MyFactory = DBAdapterFactory::MutableBinLogConcurrent<MyMemList>;

				return fLists(opt, MyFactory{	opt.db_path, opt.binlog_path1, opt.binlog_path2, syncOprions, allocator1, allocator2 },
								starting_server_with,
									"mutable concurrent binlog",
									MyMemList::getName(),
									Allocator::getName()
				);
			}else{
				using MyFactory = DBAdapterFactory::MutableConcurrent<MyMemList>;

				return fLists(opt, MyFactory{	opt.db_path, allocator1, allocator2 },
								starting_server_with,
									"mutable concurrent",
									MyMemList::getName(),
									Allocator::getName()
				);
			}
		}else{
			ArenaBuffer buffer{ allocatorSize };

			auto allocator = createAllocator(opt, buffer);

			bool const have_binlog = ! opt.binlog_path1.empty();

			if (have_binlog){
				using SyncOptions = hm4::binlogger::DiskFileBinLogger::SyncOptions;
				SyncOptions syncOprions = opt.binlog_fsync ? SyncOptions::FSYNC : SyncOptions::NONE;

				checkBinLogFile(opt.binlog_path1, opt.db_path, allocator);

				using MyFactory = DBAdapterFactory::MutableBinLog<MyMemList>;

				return fLists(opt, MyFactory{	opt.db_path, opt.binlog_path1, syncOprions, allocator },
								starting_server_with,
									"mutable binlog",
									MyMemList::getName(),
									Allocator::getName()
				);
			}else{
				using MyFactory = DBAdapterFactory::Mutable<MyMemList>;

				return fLists(opt, MyFactory{	opt.db_path, allocator },
								starting_server_with,
									"mutable",
									MyMemList::getName(),
									Allocator::getName()
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
		using MySparePool	= net::HeapSparePool;
		using MyLoop		= net::AsyncLoop<MySelector, MyWorker, MySparePool>;

		if (opt.log_level >= 2){
			logger_fmt<Logger::STARTUP>("Server start with log level {}.", opt.log_level);
		}else{
			logger_fmt<Logger::STARTUP>(
						"Server start with very low log level {}."
						"You may want to increase it at least to ERROR level.", opt.log_level
			);
		}

		getLoggerSingleton().setLevel(opt.log_level);

		if (opt.port == 0)
			printError("Can not create server socket on port zero...");

		auto const socket_options = opt.tcp_reuseport ?
				net::SOCKET_DEFAULTOPT_TCP_REUSE :
				net::SOCKET_DEFAULTOPT_TCP
		;

		int const fd = net::socket_create(net::SOCKET_TCP{}, opt.host, opt.port, opt.tcp_backlog, socket_options);

		if (fd < 0)
			printError("Can not create server socket...");

		if (opt.tcp_reuseport)
			logger<Logger::WARNING>() << "Server start with SO_REUSEPORT.";

		auto const max_clients		= std::max(opt.max_clients,	MyLoop::MIN_CLIENTS		);
		auto const buffer_capacity	= std::max(opt.buffer_capacity, MyLoop::IO_BUFFER_CAPACITY	);

		auto const max_packet_size	= round_up<size_t, 1024>(hm4::Pair::maxBytes() * 2);

		MyLoop loop{
				/* selector */	MySelector	{ max_clients },
				/* worker */	MyWorker	{ adapter_factory(), buffer_capacity },
				/* server fd */	{ fd },
				max_clients,
				opt.min_spare_pool, opt.max_spare_pool,
				opt.timeout,
				buffer_capacity,
				max_packet_size
		};

		loop.print();

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

		auto crontab_reload = mytime::CrontabControl{
			opt.crontab_reload,
			opt.crontab_min_time
		};

		auto crontab_table_maintainance = mytime::CrontabControl{
			opt.crontab_table_maintainance,
			opt.crontab_min_time
		};

		auto crontab_server_info = mytime::CrontabControl{
			opt.crontab_server_info,
			opt.crontab_min_time
		};

		while( loop.process() && signal_processing(guard()) ){
			crontab(crontab_reload,	[&adapter_factory](){
							adapter_factory().reload();
						}
			);

			crontab(crontab_table_maintainance,
						[&adapter_factory](){
							adapter_factory()->crontab();
						}
			);

			crontab(crontab_server_info,
						[&loop](){
							loop.printInfo("Server connection info");
						}
			);
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

		using MyMemList = MyDBNetMemList;

		fmt::print(
			"db_net version {version}\n"
			"\n"
			"Build:\n"
			"\tDate       : {date} {time}\n"
			"\tSelector   : {selector}\n"
			"\tConvertion : {convert}\n"
			"\tMemlist    : {memlist}\n"
			"\tAllocator  : {allocator}\n"
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
			"\t\tDo not overcommit memlist arena!\n"
			"\n"
			,

			fmt::arg("version",	hm4::version::str	),
			fmt::arg("date",	__DATE__		),
			fmt::arg("time",	__TIME__		),
			fmt::arg("selector",	MySelector::NAME	),
			fmt::arg("convert",	convert			),
			fmt::arg("memlist",	MyMemList::getName()	),
			fmt::arg("allocator",	Allocator::getName()	),
			fmt::arg("cmd",		cmd			)
		);

		fmt::print("INI File Usage:\n");

		MyOptions::print();

		fmt::print("\n");

		exit(10);
	}



	void replayBinlogFile_(std::string_view file, std::string_view path, Allocator &allocator){
		logger<Logger::WARNING>() << "Binlog file exists. Trying to replay...";

		using hm4::disk::DiskList;

		DiskList input;

		auto const result = input.openDataOnly(file, hm4::Pair::WriteOptions::ALIGNED);

		if (! result){
			logger<Logger::ERROR>() << "Replay failed.";
			return;
		}

		/* nested scope for the d-tor */
		{
			DBAdapterFactory::BinLogReplay<MyReplayList> factory{ path, allocator };

			auto &list = factory();

			// no need to show messages.
			// messages will be shown from the builder.
			for(auto const &pair : input)
				if (!pair.isKeyEmpty())
					insert(list, pair);

		} /* d-tor of list kicks here */

		logger<Logger::NOTICE>() << "Replay done.";
	}

	void replayBinlogFile(std::string_view file, std::string_view path, Allocator &allocator){
		replayBinlogFile_(file, path, allocator);
		allocator.reset();
	}

} // anonymous namespace

