#include "asyncloop.h"

#include "selector/pollselector.h"

#include "tieredmemorymanager.h"
#include "mmapbuffer.h"

using MySelector = net::selector::PollSelector;

using TMM	= memory_manager::TieredMemoryManager<MyBuffer::MMapMemoryResource>;

template<typename MyWorker>
using AsyncLoop = net::AsyncLoop<MySelector, MyWorker, TMM>;

#include "db_net_main.cc"

