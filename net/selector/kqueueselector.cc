#include "kqueueselector.h"

#include <sys/types.h>	// types for kqueue
#include <sys/event.h>	// kqueue
#include <unistd.h>	// close, for closeKQueue_()
#include <errno.h>	// errno
#include <time.h>	// struct timespec

namespace net{
namespace selector{


namespace{

	auto event2native(const FDEvent event) -> decltype(EVFILT_READ){
		switch(event){
			default:
			case FDEvent::READ	: return EVFILT_READ;
			case FDEvent::WRITE	: return EVFILT_WRITE;
		}
	}

}

// ===========================

KQueueSelector::KQueueSelector(uint32_t const maxFD) :
				fds_(maxFD){
	kqueueFD_ = kqueue();
}

KQueueSelector::KQueueSelector(KQueueSelector &&other) :
				kqueueFD_	(std::move(other.kqueueFD_	)),
				fds_	(std::move(other.fds_	)),
				fdsCount_	(std::move(other.fdsCount_	)){
	other.kqueueFD_ = -1;
}


KQueueSelector &KQueueSelector::operator =(KQueueSelector &&other){
	swap(other);

	return *this;
}

void KQueueSelector::swap(KQueueSelector &other){
	using std::swap;

	swap(kqueueFD_	, other.kqueueFD_	);
	swap(fds_	, other.fds_		);
	swap(fdsCount_	, other.fdsCount_	);
}


KQueueSelector::~KQueueSelector(){
	if (kqueueFD_ >= 0)
		::close(kqueueFD_);
}

// ===========================

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

	fdsCount_ = kevent(kqueueFD_, NULL, 0, fds_.data(), (int) fds_.size(), tsp);

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

bool KQueueSelector::insertFD(int const fd, FDEvent const event){
	if ( fdsConnected_ >= fds_.size() )
		return false;

	struct kevent ev;

	int const event2 = event2native(event);

	EV_SET(&ev, fd, event2, EV_ADD, 0, 0, nullptr);

	int const result = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	if (result < 0)
		return false;

	++fdsConnected_;
	return true;
}

bool KQueueSelector::removeFD(int const fd){
	struct kevent ev;

	EV_SET(&ev, fd, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
	int const result1 = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
	int const result2 = kevent(kqueueFD_, &ev, 1, nullptr, 0, nullptr);

	if (result1 < 0 || result2 < 0)
		return false;

	--fdsConnected_;
	return true;

}



namespace{
	FDResult getFDStatus(struct kevent const &ev){
		int const fd = (int) ev.ident;

		if (ev.flags & EV_ERROR)
			return { fd, FDStatus::ERROR };

		if ((ev.filter == EVFILT_READ) || (ev.flags & EV_EOF))
			return { fd, FDStatus::READ };

		if (ev.filter == EVFILT_WRITE)
			return { fd, FDStatus::WRITE };

		// the function can not come here,
		// but we will return FDStatus::NONE
		return { fd, FDStatus::NONE };
	}
}


} // namespace selector
} // namespace


#include "hidden_pointer_iterator.h.cc"


using namespace net::selector;


template<>
FDResult hidden_pointer_iterator<kevent, FDResult>::operator *() const{
	return getFDStatus(*pos);
}

template bool hidden_pointer_iterator<kevent, FDResult>::operator ==(hidden_pointer_iterator const &other) const;
template auto hidden_pointer_iterator<kevent, FDResult>::operator ++() -> hidden_pointer_iterator &;



auto KQueueSelector::begin() const -> iterator{
	return fds_.data();
}

auto KQueueSelector::end() const -> iterator{
	return fds_.data() + fds_.size();
}

