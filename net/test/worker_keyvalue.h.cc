#include <type_traits>

#include "worker/keyvalueworker.h"
#include "protocol/redisprotocol.h"

#include "mockdbadapter.h"

using MyProtocol	= net::protocol::RedisProtocol;

struct MyWorkerFactory{
	using Worker = net::worker::KeyValueWorker<MyProtocol, MockDBAdapter>;

	Worker operator()(){
		return { adapter };
	}

private:
	MockDBAdapter adapter;
};


