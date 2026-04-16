#include "asyncloop.h"

#include "selector/pollselector.h"

using MySelector = net::selector::PollSelector;

template<typename MyWorker, typename MySparePool>
using AsyncLoop = net::AsyncLoop<MySelector, MyWorker, MySparePool>;

#include "db_net_main.cc"

