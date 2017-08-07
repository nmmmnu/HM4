#include "multi/collectiontable.h"
#include "tableloader/directorytableloader.h"

#include "selector/pollselector.h"
#include "protocol/redisprotocol.h"
#include "worker/keyvalueworker.h"
#include "asyncloop.h"


#include "dbadapter/dbadapter.h"


struct MyImmutableDBAdapterFactory;

// ----------------------------------

constexpr const char	*HOSTNAME		= "localhost.not.used.yet";
constexpr int		PORT			= 2000;

constexpr size_t	MAX_CLIENTS		= 1024;
constexpr uint32_t	CONNECTION_TIMEOUT	= 30;
const     size_t	MAX_PACKET_SIZE		= hm4::Pair::maxBytes() * 2;

// ----------------------------------

using MySelector	= net::selector::PollSelector;
using MyProtocol	= net::protocol::RedisProtocol;
using MyAdapterFactory	= MyImmutableDBAdapterFactory;

// ----------------------------------

struct MyImmutableDBAdapterFactory{
	using MyTableLoader	= hm4::tableloader::DirectoryTableLoader;
	using MyCollectionTable	= hm4::multi::CollectionTable<MyTableLoader::container_type>;
	using MyTable		= MyCollectionTable;
	using MyDBAdapter	= DBAdapter<MyTable, MyTableLoader>;

	MyImmutableDBAdapterFactory(const char *path) :
					loader_(path),
					list_(*loader_),
					adapter_(list_, loader_){}

	MyDBAdapter &operator()(){
		return adapter_;
	}

private:
	MyTableLoader	loader_;
	MyTable		list_;
	MyDBAdapter	adapter_;
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

