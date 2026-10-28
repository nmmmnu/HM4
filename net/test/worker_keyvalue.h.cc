#include "vectorlist.h"
#include "stdallocator.h"

#include "worker/keyvalueworker.h"
#include "worker/listdbadapter.h"

#include "protocol/redisprotocol.h"

#include "simplememorymanager.h"



struct MyWorkerFactory{
	using MyProtocol	= net::protocol::RedisProtocol;

	using Allocator		= MyAllocator::STDAllocator;
	using List		= hm4::VectorList<MyAllocator::STDAllocator>;
	using MyDBAdapter	= ListDBAdapter<List>;

	using MyMemoryManager	= memory_manager::SimpleMemoryManager;

	using Worker		= net::worker::KeyValueWorker<MyProtocol, MyMemoryManager, MyDBAdapter>;

	MyMemoryManager mm;

	Worker operator()(){
		return { adapter, mm };
	}

private:
	Allocator	allocator;
	List		list{ allocator };

	std::nullptr_t	cmd;

	MyDBAdapter	adapter{ list, cmd };
};


