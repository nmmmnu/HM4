#include "pollselector.h"

#include <unistd.h>	// close
#include <errno.h>	// errno
#include <poll.h>	// poll

#include <cassert>

#include <algorithm>	// for_each

using namespace net::selector;

namespace{

	constexpr auto event2native(FDEvent const event) -> decltype(pollfd::events){
		switch(event){
		default:
		case FDEvent::READ	: return POLLIN;
		case FDEvent::WRITE	: return POLLOUT;
		}
	}

	template<class T>
	void closeFDS(T first, T last){
		std::for_each(first, last, [](pollfd const &item){
			if (item.fd >= 0)
				::close(item.fd);
		});
	}

	// pollfd createEmptyItem(){
	// 	pollfd item;
	// 	item.fd = -1;
	// 	return item;
	// }

}



PollSelector::PollSelector(uint32_t const maxFD){
	fds_.reserve(maxFD);

	//fds_(maxFD, createEmptyItem())
}

PollSelector::PollSelector(PollSelector &&other) = default;

PollSelector &PollSelector::operator =(PollSelector &&other) = default;

PollSelector::~PollSelector(){
	closeFDS(std::begin(fds_), std::end(fds_));
}



auto PollSelector::wait(int const timeout) -> WaitStatus{
	// size cast is for FreeBSD and OSX warning
	int const activity = poll(fds_.data(), (nfds_t) fds_.size(), timeout);

	if (activity < 0){
		switch(errno){
		case EAGAIN	:
		case EINTR	: return WaitStatus::OK;

		default		: return WaitStatus::ERROR;
		}
	}

	if (activity == 0)
		return WaitStatus::NONE;
	else
		return WaitStatus::OK;
}



bool PollSelector::insertFD(int const fd, FDEvent const event){
	if (fds_.size() < fds_.capacity()){
		pollfd item;

		item.fd		= fd;
		item.events	= event2native(event);
		item.revents	= 0;

		fds_.push_back(std::move(item));

		return true;
	}

	return false;
}

bool PollSelector::updateFD(int const fd, FDEvent const event){
	auto it = std::find_if(std::begin(fds_), std::end(fds_), [ fd ](pollfd const &item){
		return item.fd == fd;
	});

	if (it != std::end(fds_)){
		it->events	= event2native(event);
		it->revents	= 0;

		return true;
	}

	return false;
}

bool PollSelector::removeFD(int const fd){
	auto it = std::find_if(std::begin(fds_), std::end(fds_), [ fd ](pollfd const &item){
		return item.fd == fd;
	});

	if (it != fds_.end()){
		if (it != std::prev(fds_.end()))
			*it = std::move(fds_.back());

		fds_.pop_back();

		return true;
	}

	return false;
}



namespace{
	FDResult getFDStatus(pollfd const &p){
		int const fd = p.fd;

		if (fd >= 0){
			if (p.revents & POLLERR)
				return { fd, FDStatus::ERROR };

			if (p.revents & POLLIN)
				return { fd, FDStatus::READ };

			if (p.revents & POLLOUT)
				return { fd, FDStatus::WRITE };
		}

		return { fd, FDStatus::NONE };
	}
}




void PollSelector::HPI::inc(const value_type * &a){
	++a;
}

FDResult PollSelector::HPI::conv(const value_type *a){
	return getFDStatus(*a);
}



auto PollSelector::begin() const -> iterator{
	return fds_.data();
}

auto PollSelector::end() const -> iterator{
	return fds_.data() + fds_.size();
}


