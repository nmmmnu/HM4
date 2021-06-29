#ifndef NET_SELECTOR_H_
#define NET_SELECTOR_H_

	#if SELECTOR == epoll

		#include "epollselector.h"

		#define selector__	EPollSelector;

	#elseif SELECTOR == kqueue

		#include "kqueueselector.h"

		#define selector__	KQueueSelector;

	#else

		#include "pollselector.h"

		#define selector__	PollSelector;

	#endif

namespace net{
namespace selector{

using MySelector = selector__;

} // namespace selector
} // namespace

#unset selector__

#endif

