#ifndef NET_CONNECTION_LIMITER_SELECTOR_H_
#define NET_CONNECTION_LIMITER_SELECTOR_H_

#include "selectordefs.h"

namespace net{
namespace selector{

template<class Selector>
class ConnectionLimiter{
public:
	using iterator = typename Selector::iterator;

	ConnectionLimiter(uint32_t const connectedClientsMax) : connectedClientsMax_(connectedClientsMax){
	}

	bool insertFD(int fd, FDEvent event = FDEvent::READ){
	}

	bool updateFD(int fd, FDEvent event){
	}

	bool removeFD(int fd){
	}

	WaitStatus wait(int const timeout){
	}

	iterator begin() const{
	}

	iterator end() const{
	}

private:
	Selector	selector_;
	uint32_t	connectedClients_	= 0;
	uint32_t	connectedClientsMax_	= 0;
};


} // namespace selector
} // namespace

#endif

