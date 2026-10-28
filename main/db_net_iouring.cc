#define IO_ENGINE

#include "asynccompletionloop.h"

#include "ioengine/iouringengine.h"

#include "tieredmemorymanager.h"
#include "mmapbuffer.h"

using MyEngine	= net::ioengine::IOURingEngine;

using TMM	= memory_manager::TieredMemoryManager<MyBuffer::MMapMemoryResource>;

template<typename MyWorker>
using AsyncLoop = net::AsyncCompletionLoop<MyEngine, MyWorker, TMM>;

#include "db_net_main.cc"

