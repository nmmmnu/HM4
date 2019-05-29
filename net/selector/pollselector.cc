#include "pollselector.h"

#include <poll.h>	// poll
#include <unistd.h>	// close, for closeStatusData_()
#include <errno.h>	// errno

namespace net{
namespace selector{


namespace{

auto pollConvert(const FDEvent event) -> decltype(pollfd::events){
	switch(event){
		default:
		case FDEvent::READ	: return POLLIN;
		case FDEvent::WRITE	: return POLLOUT;
	}
}

}

// ===========================

PollSelector::PollSelector(uint32_t const maxFD) :
				statusData_(maxFD){
	initializeStatusData_();
}

PollSelector::PollSelector(PollSelector &&other) = default;

PollSelector &PollSelector::operator =(PollSelector &&other) = default;

PollSelector::~PollSelector(){
	closeStatusData_();
}

// ===========================

uint32_t PollSelector::maxFD() const{
	return (uint32_t) statusData_.size();
}

WaitStatus PollSelector::wait(int const timeout){
	// size cast is for FreeBSD and OSX warning
	int const activity = poll(statusData_.data(), (nfds_t) statusData_.size(), timeout);

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

FDResult PollSelector::getFDStatus(uint32_t const no) const{
	const auto &p = statusData_[no];
	int  const fd = p.fd;
	auto const ev = p.revents;

	if (ev & POLLERR)
		return { fd, FDStatus::ERROR };

	if (ev & POLLIN)
		return { fd, FDStatus::READ };

	if (ev & POLLOUT)
		return { fd, FDStatus::WRITE };

	return { fd, FDStatus::NONE };
}

bool PollSelector::insertFD(int const fd, const FDEvent event){
	uint32_t pos = 0;
	bool     pok = false;

	for(uint32_t i = 0; i < statusData_.size(); ++i){
		if (statusData_[i].fd == fd){
			pos = i;
			pok = true;
			break;
		}

		if (pok == false && statusData_[i].fd < 0){
			pos = i;
			pok = true;
		}
	}

	if (! pok)
		return false;

	statusData_[pos].fd = fd;
	statusData_[pos].events = pollConvert(event);

	return true;
}

bool PollSelector::updateFD(int const fd, const FDEvent event){
	// bit ugly.
	for(auto &item : statusData_)
		if (item.fd == fd){
			item.events = pollConvert(event);
			return true;
		}

	return false;
}

bool PollSelector::removeFD(int const fd){
	// bit ugly.
	for(auto &item : statusData_)
		if (item.fd == fd){
			item.fd = -1;
			return true;
		}

	return false;
}

// ===========================

void PollSelector::initializeStatusData_(){
	for(auto &item : statusData_)
		item.fd = -1;
}

void PollSelector::closeStatusData_(){
	for(auto &item : statusData_)
		if (item.fd >= 0)
			::close(item.fd);
}


} // namespace selector
} // namespace

