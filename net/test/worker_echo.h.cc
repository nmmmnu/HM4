#include "worker/echoworker.h"

template<typename>
struct MyWorkerFactoryT{
	using Worker = net::worker::EchoWorker;

	Worker operator()(){
		return {};
	}
};


