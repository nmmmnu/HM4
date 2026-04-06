#include "haikuselector.h"

#include <unistd.h>	// close, for closeStatusData_()
#include <errno.h>	// errno
#include <OS.h>		//

#include <cassert>

#include <algorithm>	// for_each

using namespace net::selector;

namespace{

	constexpr auto event2native(FDEvent const event) -> decltype(object_wait_info::events){
		switch(event){
		default:
		case FDEvent::READ	: return B_EVENT_READ;
		case FDEvent::WRITE	: return B_EVENT_WRITE;
		}
	}

	template<class T>
	void closeFDS(T first, T last){
		std::for_each(first, last, [](object_wait_info const &item){
			if (item.object >= 0)
				::close(item.object);
		});
	}
}



auto HaikuSelector::SparseMapController::getKey(mapped_type const &value) -> key_type{
	return static_cast<key_type>(value.object);
}

auto HaikuSelector::SparseMapController::getVal(mapped_type const &value) -> value_type const &{
	return value;
}

auto HaikuSelector::SparseMapController::getVal(mapped_type       &value) -> value_type &{
	return value;
}



HaikuSelector::HaikuSelector(uint32_t const conf_rlimitNoFile, uint32_t const conf_max_clients) :
					fds_(conf_rlimitNoFile, conf_max_clients){}

HaikuSelector::HaikuSelector(HaikuSelector &&other) = default;

HaikuSelector &HaikuSelector::operator =(HaikuSelector &&other) = default;

HaikuSelector::~HaikuSelector(){
	closeFDS(std::begin(fds_), std::end(fds_));
}



auto HaikuSelector::wait(int const timeout) -> WaitStatus{
	auto const status = wait_for_objects_etc(fds_.data(), (int) fds_.size(), B_RELATIVE_TIMEOUT, bigtime_t{ timeout } * 1'000);

	if (status < B_OK){
		switch(status) {

		case B_INTERRUPTED:
		case B_WOULD_BLOCK: 	return WaitStatus::OK;
		case B_TIMED_OUT:	return WaitStatus::NONE;

		// B_BAD_VALUE etc.
		default:  		return WaitStatus::ERROR;
		}
	}

	return WaitStatus::OK;
}



bool HaikuSelector::insertFD(int const fd, FDEvent const event){
	if (fds_.size() < fds_.capacity()){
		object_wait_info item;

		item.object	= fd;
		item.type	= B_OBJECT_TYPE_FD;
		item.events	= event2native(event);

		fds_.insert(std::move(item));

		return true;
	}

	return false;
}

bool HaikuSelector::updateFD(int const fd, FDEvent const event){
	if (auto *it = fds_.find(static_cast<uint32_t>(fd)); it){
		it->events	= event2native(event);

		return true;
	}

	return false;
}

bool HaikuSelector::removeFD(int const fd){
	return fds_.remove(static_cast<uint32_t>(fd));
}



namespace{
	FDResult getFDStatus(object_wait_info const &p){
		int const fd = p.object;

		assert(fd >= 0);

		if (p.events & (B_EVENT_INVALID | B_EVENT_ERROR))
			return { fd, FDStatus::ERROR };

		if (p.events & B_EVENT_READ)
			return { fd, FDStatus::READ };

		if (p.events & B_EVENT_WRITE)
			return { fd, FDStatus::WRITE };

		return { fd, FDStatus::NONE };
	}
}




void HaikuSelector::HPI::inc(const value_type * &a){
	++a;
}

FDResult HaikuSelector::HPI::conv(const value_type *a){
	return getFDStatus(*a);
}



auto HaikuSelector::begin() const -> iterator{
	return fds_.data();
}

auto HaikuSelector::end() const -> iterator{
	return fds_.data() + fds_.size();
}


