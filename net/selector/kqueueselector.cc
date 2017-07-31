#include "kqueueselector.h"

#include <sys/types.h>	// types for kqueue
#include <sys/event.h>	// kqueue
#include <unistd.h>	// close, for closeKQueue_()
#include <time.h>	// struct timespec

namespace net{
namespace selector{


namespace{

auto kQueueConvert(const FDEvent event) -> decltype(EVFILT_READ){
	switch(event){
		default:
		case FDEvent::READ	: return EVFILT_READ;
		case FDEvent::WRITE	: return EVFILT_WRITE;
	}
}

}

// ===========================

KQueueSelector::KQueueSelector(uint32_t const maxFD) :
				statusData_(maxFD){
	initializeKQueue_();
}

KQueueSelector::KQueueSelector(KQueueSelector &&other) :
				kqueueFD_	(std::move(other.kqueueFD_	)),
				statusData_	(std::move(other.statusData_	)),
				statusCount_	(std::move(other.statusCount_	)){
	other.kqueueFD_ = -1;
}


KQueueSelector &KQueueSelector::operator =(KQueueSelector &&other){
	swap(other);

	return *this;
}

void KQueueSelector::swap(KQueueSelector &other){
	using std::swap;

	swap(kqueueFD_		, other.kqueueFD_	);
	swap(statusData_	, other.statusData_	);
	swap(statusCount_	, other.statusCount_	);
}


KQueueSelector::~KQueueSelector(){
	closeKQueue_();
}

// ===========================

uint32_t KQueueSelector::maxFD() const{
	return (uint32_t) statusData_.size();
}

WaitStatus KQueueSelector::wait(int const timeout){
	if (kqueueFD_ < 0)
		return WaitStatus::ERROR;

	struct timespec ts;
	struct timespec *tsp = nullptr;

	if (timeout >= 0){
		ts.tv_sec  = timeout / 1000;	// sec
		ts.tv_nsec = timeout % 1000;	// nanoseconds
		tsp = &ts;
	}

	statusCount_ = kevent(kqueueFD_, NULL, 0, statusData_.data(), (int) statusData_.size(), tsp);

	if (statusCount_ < 0)
		return WaitStatus::ERROR;

	if (statusCount_ == 0)
		return WaitStatus::NONE;

	return WaitStatus::OK;
}

FDResult KQueueSelector::getFDStatus(uint32_t const no) const{
	if (no < (uint32_t) statusCount_){
		const struct kevent &ev = statusData_[no];

		int const fd = (int) ev.ident;

		if (ev.flags & EV_ERROR)
			return { fd, FDStatus::ERROR };

		if ((ev.filter == EVFILT_READ) || (ev.flags & EV_EOF))
			return { fd, FDStatus::READ };

		if (ev.filter == EVFILT_WRITE)
			return { fd, FDStatus::WRITE };
	}

	return FDStatus::STOP;
}

bool KQueueSelector::insertFD(int const fd, FDEvent const event){
	struct kevent ev;

	int const event2 = kQueueConvert(event);

	EV_SET(&ev, fd, event2, EV_ADD, 0, 0, nullptr);

	int const result = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	return result >= 0;
}

bool KQueueSelector::removeFD(int const fd){
	struct kevent ev;

	EV_SET(&ev, fd, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
	int const result1 = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
	int const result2 = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	return result1 >= 0 && result2 >= 0;
}

// ===========================

void KQueueSelector::initializeKQueue_(){
	kqueueFD_ = kqueue();
}

void KQueueSelector::closeKQueue_(){
	::close(kqueueFD_);
}


} // namespace selector
} // namespace

