#include "asyncloop.h"

// use POLL-style loop

#if defined SELECTOR_EPOLL
	#include "selector/epollselector.h"

	using MySelector	= net::selector::EPollSelector;
#elif defined SELECTOR_KQUEUE
	#include "selector/kqueueselector.h"

	using MySelector	= net::selector::KQueueSelector;
#elif defined SELECTOR_HAIKU
	#include "selector/haikuselector.h"

	using MySelector	= net::selector::HaikuSelector;
#elif defined SELECTOR_POLL
	#include "selector/pollselector.h"

	using MySelector	= net::selector::PollSelector;
#else
	#error "No net::selector selected!"
#endif

template<typename MyWorker, typename MySparePool>
using AsyncLoop = net::AsyncLoop<MySelector, MyWorker, MySparePool>;

#include "db_net_main.cc"

