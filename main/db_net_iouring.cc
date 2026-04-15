#define IO_ENGINE

#include "asynccompletionloop.h"

#include "ioengine/iouringengine.h"

using MyEngine = net::ioengine::IOURingEngine;

template<typename MyWorker, typename MySparePool>
using AsyncLoop = net::AsyncCompletionLoop<MyEngine, MyWorker, MySparePool>;

#include "db_net_main.cc"

