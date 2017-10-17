#include "worker/echoworker.h"

struct MyWorkerFactory{
	using Worker = net::worker::EchoWorker;

	Worker operator()(){
		return {};
	}
};

#include "test_selector.h.cc"

