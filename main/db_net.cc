#include "multi/collectionlist.h"
#include "listloader/directorylistloader.h"

#include "selector/pollselector.h"
#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"


#include "dbadapter/imutabledbadapter.h"

#include "multi/duallist.h"
#include "skiplist.h"
#include "dbadapter/mutabledbadapter.h"


struct MyImmutableDBAdapterFactory;
struct MyMutableDBAdapterFactory;

// ----------------------------------

constexpr const char	*HOSTNAME		= "localhost.not.used.yet";
constexpr int		PORT			= 2000;

constexpr size_t	MAX_CLIENTS		= 1024;
constexpr uint32_t	CONNECTION_TIMEOUT	= 30;
const     size_t	MAX_PACKET_SIZE		= hm4::Pair::maxBytes() * 2;

// ----------------------------------

using MySelector	= net::selector::PollSelector;
using MyProtocol	= net::protocol::RedisProtocol;
using MyAdapterFactory	= MyMutableDBAdapterFactory;

// ----------------------------------

struct MyImmutableDBAdapterFactory{
	using MyListLoader	= hm4::listloader::DirectoryListLoader;
	using MyImmutableList	= hm4::multi::CollectionList<MyListLoader::container_type>;
	using MyList		= MyImmutableList;
	using MyDBAdapter	= ImutableDBAdapter<MyList, MyListLoader>;

	MyImmutableDBAdapterFactory(const char *path) :
					loader_(path),
					imList_(*loader_),
					adapter_(imList_, loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	MyListLoader				loader_;
	MyImmutableList		imList_;
	MyDBAdapter		adapter_;
};

struct MyMutableDBAdapterFactory{
	using MyListLoader	= hm4::listloader::DirectoryListLoader;
	using MyImmutableList	= hm4::multi::CollectionList<MyListLoader::container_type>;
	using MyMutableList	= hm4::SkipList;
	using MyList		= hm4::multi::DualList<MyMutableList, MyImmutableList, true>;
	using MyDBAdapter	= MutableDBAdapter<MyList, MyListLoader>;

	MyMutableDBAdapterFactory(const char *path) :
					loader_(path),
					imList_(*loader_),
					list_(muList_, imList_),
					adapter_(list_, loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	MyListLoader					loader_;
	MyImmutableList			imList_;
	MyMutableList				muList_;
	MyList				list_;
	MyDBAdapter		adapter_;
};

static int printUsage(const char *cmd);


int main(int argc, char **argv){
	if (argc <= 1)
		return printUsage(argv[0]);

	const auto path = argv[1];

	// ----------------------------------

	MyAdapterFactory adapter_f(path);

	using MyDBAdapter	= MyAdapterFactory::MyDBAdapter;
	using MyWorker		= net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;
	using MyLoop		= net::AsyncLoop<MySelector, MyWorker>;

	int const fd1 = net::socket_create(net::SOCKET_TCP{}, HOSTNAME, PORT);

	MyLoop loop( MySelector{ MAX_CLIENTS }, MyWorker{ adapter_f() }, { fd1 },
							CONNECTION_TIMEOUT, MAX_PACKET_SIZE);

	while(loop.process());
}


#include <iostream>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " [lsm_path] - start server"		<< '\n'

		<< "\t\tPath names must be written without extention"		<< '\n'
		<< "\t\tExample 'directory/file.*.db'"				<< '\n'

		<< '\n';

	return 10;
}

