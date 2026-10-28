#include "asyncloop.h"

#include "selector/pollselector.h"

#include "tieredmemorymanager.h"

using MySelector = net::selector::PollSelector;

using TSM	= memory_manager::TieredMemoryManager;

template<typename MyWorker>
using AsyncLoop = net::AsyncLoop<MySelector, MyWorker, TSM>;

#include "db_net_main.cc"

