#include "epollselector.h"

#include <unistd.h>	// close, for closeEPoll_()
#include <errno.h>	// errno

namespace net{
namespace selector{


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



EPollSelector::EPollSelector(uint32_t const maxFD) : fds_(maxFD){
	epollFD_ = epoll_create((int) fds_.size());
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

	swap(epollFD_		, other.epollFD_	);
	swap(fds_		, other.fds_		);
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
		case EINTR	: return WaitStatus::OK;

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

	return epoll_ctl(epollFD_, EPOLL_CTL_ADD, fd, &ev) >= 0;
}

bool EPollSelector::updateFD(int const fd, FDEvent const event){
	epoll_event ev = create_epoll_event(fd, event);

	return epoll_ctl(epollFD_, EPOLL_CTL_MOD, fd, &ev) >= 0;
}

bool EPollSelector::removeFD(int const fd){
	return epoll_ctl(epollFD_, EPOLL_CTL_DEL, fd, nullptr) >= 0;
}



FDResult EPollSelector::getFDStatus(epoll_event const &p){
	int const fd = p.data.fd;

	if (p.events & EPOLLERR)
		return { fd, FDStatus::ERROR };

	if ((p.events & EPOLLIN) || (p.events & EPOLLHUP))
		return { fd, FDStatus::READ };

	if (p.events & EPOLLOUT)
		return { fd, FDStatus::WRITE };

	// the function can not come here,
	// but we will return FDStatus::NONE
	return { fd, FDStatus::NONE };
}




} // namespace selector
} // namespace

