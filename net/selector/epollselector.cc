#include "epollselector.h"

#include <sys/epoll.h>	// epoll
#include <unistd.h>	// close, for closeEPoll_()
#include <errno.h>	// errno

namespace net{
namespace selector{


namespace{

auto epollConvert(const FDEvent event) -> decltype(epoll_event::events){
	switch(event){
		default:
		case FDEvent::READ	: return EPOLLIN;
		case FDEvent::WRITE	: return EPOLLOUT;
	}
}

}

// ===========================

EPollSelector::EPollSelector(uint32_t const maxFD) :
				statusData_(maxFD){
	initializeEPoll_();
}

EPollSelector::EPollSelector(EPollSelector &&other) :
				epollFD_	(std::move(other.epollFD_	)),
				statusData_	(std::move(other.statusData_	)),
				statusCount_	(std::move(other.statusCount_	)){
	other.epollFD_ = -1;
}


EPollSelector &EPollSelector::operator =(EPollSelector &&other){
	swap(other);

	return *this;
}

void EPollSelector::swap(EPollSelector &other){
	using std::swap;

	swap(epollFD_		, other.epollFD_	);
	swap(statusData_	, other.statusData_	);
	swap(statusCount_	, other.statusCount_	);
}


EPollSelector::~EPollSelector(){
	closeEPoll_();
}

// ===========================

uint32_t EPollSelector::maxFD() const{
	return (uint32_t) statusData_.size();
}

WaitStatus EPollSelector::wait(int const timeout){
	if (epollFD_ < 0)
		return WaitStatus::ERROR;

	statusCount_ = epoll_wait(epollFD_, statusData_.data(), (int) statusData_.size(), timeout);

	if (statusCount_ < 0){
		switch(errno){
		case EINTR	: return WaitStatus::OK;

		default		: return WaitStatus::ERROR;
		}
	}

	if (statusCount_ == 0)
		return WaitStatus::NONE;
	else
		return WaitStatus::OK;
}

FDResult EPollSelector::getFDStatus(uint32_t const no) const{
	if (no < (uint32_t) statusCount_){
		const epoll_event &ev = statusData_[no];

		int const fd = ev.data.fd;

		if (ev.events & EPOLLERR)
			return { fd, FDStatus::ERROR };

		if ((ev.events & EPOLLIN) || (ev.events & EPOLLHUP))
			return { fd, FDStatus::READ };

		if (ev.events & EPOLLOUT)
			return { fd, FDStatus::WRITE };
	}

	return FDStatus::STOP;
}

bool EPollSelector::insertFD(int const fd, FDEvent const event){
	return mutateFD_(fd, event, EPOLL_CTL_ADD);
}

bool EPollSelector::updateFD(int const fd, FDEvent const event){
	return mutateFD_(fd, event, EPOLL_CTL_MOD);
}

bool EPollSelector::mutateFD_(int const fd, FDEvent const event, int const op){
	epoll_event ev;
	ev.events = epollConvert(event);
	ev.data.fd = fd;

	int const result = epoll_ctl(epollFD_, op, fd, &ev);

	return result >= 0;
}

bool EPollSelector::removeFD(int const fd){
	int const result = epoll_ctl(epollFD_, EPOLL_CTL_DEL, fd, nullptr);

	return result >= 0;
}

// ===========================

void EPollSelector::initializeEPoll_(){
	epollFD_ = epoll_create((int) statusData_.size());
}

void EPollSelector::closeEPoll_(){
	::close(epollFD_);
}


} // namespace selector
} // namespace

