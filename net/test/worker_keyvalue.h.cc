#include "vectorlist.h"
#include "stdallocator.h"

#include "worker/keyvalueworker.h"
#include "worker/listdbadapter.h"

#include "protocol/redisprotocol.h"


struct MyWorkerFactory{
	using MyProtocol	= net::protocol::RedisProtocol;

	using Allocator		= MyAllocator::STDAllocator;
	using List		= hm4::VectorList<MyAllocator::STDAllocator>;
	using MyDBAdapter	= ListDBAdapter<List>;

	using Worker = net::worker::KeyValueWorker<MyProtocol, MyDBAdapter>;

	Worker operator()(){
		return { adapter, net::IOBuffer::INITIAL_RESERVE };
	}

private:
	Allocator	allocator;
	List		list{ allocator };

	std::nullptr_t	cmd;

	MyDBAdapter	adapter{ list, cmd, cmd };
};


