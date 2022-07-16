#include "kqueueselector.h"

#include <sys/types.h>	// types for kqueue
#include <sys/event.h>	// kqueue
#include <unistd.h>	// close, for closeKQueue_()
#include <errno.h>	// errno
#include <time.h>	// struct timespec

using namespace net::selector;

KQueueSelector::KQueueSelector(size_t const server_limit) : fds_(server_limit){
	kqueueFD_ = kqueue();
}

KQueueSelector::KQueueSelector(KQueueSelector &&other) :
				kqueueFD_	(std::move(other.kqueueFD_	)),
				fds_		(std::move(other.fds_		)),
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



namespace{
	template<typename EVFILT, typename EV>
	auto k_op_apply(int const kfd, int const fd, EVFILT const filter, EV const op){
		struct kevent ev;

		EV_SET(&ev, fd, filter, op, 0, 0, nullptr);
		return kevent(kfd, &ev, 1, nullptr, 0, nullptr);
	}

	template<typename EV>
	bool k_op(int const kfd, int const fd, EV const op1, EV const op2){
		int const result1 = k_op_apply(kfd, fd, EVFILT_READ,  op1);
		int const result2 = k_op_apply(kfd, fd, EVFILT_WRITE, op2);

		return result1 >=0 && result2 >= 0;
	}

	template<typename EV>
	bool k_op(int const kfd, int const fd, EV const op){
		return k_op(kfd, fd, op, op);
	}

	bool k_add(int const kfd, int const fd){
		return k_op(kfd, fd, EV_ADD);
	}

	bool k_rem(int const kfd, int const fd){
		return k_op(kfd, fd, EV_DELETE);
	}

	template<FDEvent E>
	auto makeEV(FDEvent const event){
		return E == event ? EV_ENABLE : EV_DISABLE;
	}

	bool k_update(int const kfd, int const fd, FDEvent const event){
		return k_op(
				kfd, fd,
				makeEV<FDEvent::READ>(event),
				makeEV<FDEvent::WRITE>(event)
		);
	}
}



bool KQueueSelector::insertFD(int const fd, FDEvent const event){
	return k_add(kqueueFD_, fd) && k_update(kqueueFD_, fd, event);
}

bool KQueueSelector::updateFD(int const fd, FDEvent const event){
	return k_update(kqueueFD_, fd, event);
}

bool KQueueSelector::removeFD(int const fd){
	return k_rem(kqueueFD_, fd);
}



namespace{
	FDResult getFDStatus(struct kevent const &ev){
		int const fd = (int) ev.ident;

		if (ev.flags & EV_ERROR)
			return { fd, FDStatus::ERROR };

		if (ev.filter == EVFILT_READ) // ) || (ev.flags & EV_EOF)
			return { fd, FDStatus::READ };

		if (ev.filter == EVFILT_WRITE)
			return { fd, FDStatus::WRITE };

		// the function can not come here,
		// but we will return FDStatus::NONE
		return { fd, FDStatus::NONE };
	}
}



void KQueueSelector::HPI::inc(const value_type * &a){
	++a;
}

FDResult KQueueSelector::HPI::conv(const value_type *a){
	return getFDStatus(*a);
}



auto KQueueSelector::begin() const -> iterator{
	return fds_.data();
}

auto KQueueSelector::end() const -> iterator{
	return fds_.data() + fds_.size();
}

