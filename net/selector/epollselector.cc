#include "epollselector.h"

#include <unistd.h>	// close, for closeEPoll_()
#include <errno.h>	// errno

#include <sys/epoll.h>

using namespace net::selector;

namespace{

	constexpr auto event2native(const FDEvent event) -> decltype(epoll_event::events){
		switch(event){
		default:
		case FDEvent::READ	: return EPOLLIN;
		case FDEvent::WRITE	: return EPOLLOUT;
		}
	}

	epoll_event create_epoll_event(int const fd, FDEvent const event){
		epoll_event ev;

		ev.events = event2native(event);
		ev.data.fd = fd;

		return ev;
	}

}



EPollSelector::EPollSelector(size_t const server_limit) : fds_(server_limit){
	epollFD_ = epoll_create( /* epoll_ignores_this_argument */ 1 );
}

EPollSelector::EPollSelector(EPollSelector &&other) :
				epollFD_	(std::move(other.epollFD_	)),
				fds_		(std::move(other.fds_		)),
				fdsCount_	(std::move(other.fdsCount_	)){
	other.epollFD_ = -1;
}

EPollSelector &EPollSelector::operator =(EPollSelector &&other){
	swap(other);

	return *this;
}

void EPollSelector::swap(EPollSelector &other){
	using std::swap;

	swap(epollFD_	, other.epollFD_	);
	swap(fds_	, other.fds_		);
	swap(fdsCount_	, other.fdsCount_	);
}

EPollSelector::~EPollSelector(){
	if (epollFD_ >= 0)
		::close(epollFD_);
}



WaitStatus EPollSelector::wait(int const timeout){
	if (epollFD_ < 0)
		return WaitStatus::ERROR;

	fdsCount_ = epoll_wait(epollFD_, fds_.data(), (int) fds_.size(), timeout);

	if (fdsCount_ < 0){
		switch(errno){
		case EINTR	: return WaitStatus::NONE;

		default		: return WaitStatus::ERROR;
		}
	}

	if (fdsCount_ == 0)
		return WaitStatus::NONE;
	else
		return WaitStatus::OK;
}



bool EPollSelector::insertFD(int const fd, FDEvent const event){
	epoll_event ev = create_epoll_event(fd, event);

	int const result = epoll_ctl(epollFD_, EPOLL_CTL_ADD, fd, &ev);

	return result == 0;
}

bool EPollSelector::updateFD(int const fd, FDEvent const event){
	epoll_event ev = create_epoll_event(fd, event);

	int const result = epoll_ctl(epollFD_, EPOLL_CTL_MOD, fd, &ev);

	return result == 0;
}

bool EPollSelector::removeFD(int const fd){
	int const result = epoll_ctl(epollFD_, EPOLL_CTL_DEL, fd, nullptr);

	return result == 0;
}


namespace {
	FDResult getFDStatus(epoll_event const &ev){
		int const fd = ev.data.fd;

		if (ev.events & EPOLLERR)
			return { fd, FDStatus::ERROR };

		if ((ev.events & EPOLLIN) || (ev.events & EPOLLHUP))
			return { fd, FDStatus::READ };

		if (ev.events & EPOLLOUT)
			return { fd, FDStatus::WRITE };

		// the function can not come here,
		// but we will return FDStatus::NONE
		return { fd, FDStatus::NONE };
	}
}



void EPollSelector::HPI::inc(const value_type * &a){
	++a;
}

FDResult EPollSelector::HPI::conv(const value_type *a){
	return getFDStatus(*a);
}



auto EPollSelector::begin() const -> iterator{
	return fds_.data();
}

auto EPollSelector::end() const -> iterator{
	return fds_.data() + fdsCount_;
}

