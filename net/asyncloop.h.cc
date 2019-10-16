#include "sockets.h"
#include "mynarrow.h"

#include "worker/workerdefs.h"

#include <unistd.h>	// read
#include <algorithm>	// find
#include <iostream>

namespace net{

namespace impl_{
	template<class T>
	bool findFD(T const &c, int const fd){
		return std::find(std::begin(c), std::end(c), fd) != std::end(c);
	}
}

template<class SELECTOR, class WORKER>
AsyncLoop<SELECTOR, WORKER>::AsyncLoop(SELECTOR &&selector, WORKER &&worker, const std::initializer_list<int> &serverFD,
			uint32_t const conf_maxClients		,
			uint32_t const conf_connectionTimeout	,
			size_t   const conf_maxPacketSize
		) :
					selector_(std::move(selector)),
					worker_(std::move(worker)),
					serverFD_(serverFD),

					conf_maxClients		(	max__(conf_maxClients,		MIN_CLIENTS		)),
					conf_connectionTimeout_	(	max__(conf_connectionTimeout,	CONNECTION_TIMEOUT	)),
					conf_maxPacketSize_	(	max__(conf_maxPacketSize,	BUFFER_CAPACITY		)){
	for(int const fd : serverFD_){
		socket_makeNonBlocking(fd);
		selector_.insertFD(fd);
	}
}

// ===========================

template<class SELECTOR, class WORKER>
bool AsyncLoop<SELECTOR, WORKER>::process(){
	using WaitStatus = selector::WaitStatus;
	using FDStatus   = selector::FDStatus;

	keepProcessing_ = true;

	log_("poll()-ing...");
	const WaitStatus status = selector_.wait(WAIT_TIMEOUT_MS);

	if (status == WaitStatus::ERROR){
		log_("poll() error");

		return false;
	}

	if (status == WaitStatus::NONE){
		// idle loop, check for expired conn
		expireFD_();
		return true;
	}

	for(auto const &t : selector_){
		switch(t.status){
		case FDStatus::READ:
			handleRead_( t.fd );
			break;

		case FDStatus::WRITE:
			handleWrite_( t.fd );
			break;

		case FDStatus::ERROR:
			handleDisconnect_( t.fd, DisconnectStatus::ERROR );
			break;

		case FDStatus::STOP:
			goto break2;

		case FDStatus::NONE:
		default:
			break;

		}
	}

	break2: // label for goto... ;)

	return keepProcessing_;
}

// ===========================

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::handleRead_(int const fd){
	if ( impl_::findFD(serverFD_, fd) ){
		while (handleConnect_(fd));
		return;
	}

	// -------------------------------------

	auto it = clients_.find(fd);

	if (it == clients_.end())
		return handleDisconnect_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &buffer = it->second;

	// -------------------------------------

	ssize_t const size = ::read(fd, inputBuffer_, BUFFER_CAPACITY);

	if (size <= 0)
		return handleSocketOps_(fd, size);

	if (buffer.size() + narrow<size_t>(size) > conf_maxPacketSize_)
		return handleDisconnect_(fd, DisconnectStatus::ERROR);

	// size is checked already
	buffer.push( static_cast<size_t>(size), inputBuffer_);

	buffer.restartTimer();

	handleWorker_(fd, buffer);
}

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::handleWrite_(int const fd){
	if ( impl_::findFD(serverFD_, fd) ){
		// WTF?!?
		return;
	}

	// -------------------------------------

	auto it = clients_.find(fd);

	if (it == clients_.end())
		return handleDisconnect_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	ClientBuffer &buffer = it->second;

	// -------------------------------------

	if (buffer.size() > 0){
		// available == 0 - this might happen on MacOS with kqueue()

		ssize_t const size = ::write(fd, buffer.data(), buffer.size());

		if (size <= 0)
			return handleSocketOps_(fd, size);

		// size is checked already
		buffer.pop( static_cast<size_t>(size) );

		buffer.restartTimer();
	}

	if (buffer.size() == 0){
		// process with read
		selector_.updateFD(fd, selector::FDEvent::READ);

		return;
	}
}

template<class SELECTOR, class WORKER>
bool AsyncLoop<SELECTOR, WORKER>::handleWorker_(int const fd, ClientBuffer &buffer){
	const WorkerStatus status = worker_( buffer );

	switch( status ){
	case WorkerStatus::PASS:
		return false;

	case WorkerStatus::READ:
		selector_.updateFD(fd, selector::FDEvent::READ);

		return true;

	case WorkerStatus::WRITE:
		selector_.updateFD(fd, selector::FDEvent::WRITE);

		return true;

	case WorkerStatus::DISCONNECT:
		handleDisconnect_(fd, DisconnectStatus::WORKER_NORMAL);

		return true;

	case WorkerStatus::SHUTDOWN:
	//	handleDisconnect_(fd, DisconnectStatus::WORKER_NORMAL);
		keepProcessing_ = false;

		return true;

	default:
	case WorkerStatus::DISCONNECT_ERROR:
		handleDisconnect_(fd, DisconnectStatus::WORKER_ERROR);

		return true;
	}
}

template<class SELECTOR, class WORKER>
bool AsyncLoop<SELECTOR, WORKER>::handleConnect_(int const fd){
	// fd is same as serverFD_
	int const newFD = socket_accept(fd);

	// serverFD_ is non blocking, so we do not need to check EAGAIN
	if (newFD < 0)
		return false;

	if ( clients_.size() < conf_maxClients && insertFD_(newFD) ){
		// socket_makeNonBlocking(newFD);

		log_("Connect", newFD);
	}else{
		socket_close(newFD);

		log_("Drop connect", newFD);
	}

	return true;
}

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::handleDisconnect_(int const fd, const DisconnectStatus error){
	removeFD_(fd);

	socket_close(fd);

	switch(error){
	case DisconnectStatus::NORMAL			: return log_("Normal  Disconnect",				fd);
	case DisconnectStatus::ERROR			: return log_("Error   Disconnect",				fd);
	case DisconnectStatus::TIMEOUT			: return log_("Timeout Disconnect",				fd);

	case DisconnectStatus::WORKER_NORMAL		: return log_("Worker  Disconnect - Normal",			fd);
	case DisconnectStatus::WORKER_ERROR		: return log_("Worker  Disconnect - Error",			fd);

	case DisconnectStatus::PROBLEM_MAP_NOT_FOUND	: return log_("Problem Disconnect - FD not found",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_READ	: return log_("Problem Disconnect - Read buffer full",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_WRITE	: return log_("Problem Disconnect - Write buffer problem",	fd);
	};
}

// ===========================

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::handleSocketOps_(int const fd, ssize_t const size){
	if (size < 0){
		if ( socket_check_eagain() ){
			// try again
			return;
		}else{
			// error, disconnect.
			return handleDisconnect_(fd, DisconnectStatus::ERROR);
		}
	}

	if (size == 0){
		// normal, disconnect.
		return handleDisconnect_(fd, DisconnectStatus::NORMAL);
	}
}

// ===========================

template<class SELECTOR, class WORKER>
bool AsyncLoop<SELECTOR, WORKER>::insertFD_(int const fd){
	if ( ! selector_.insertFD(fd) )
		return false;

	clients_[fd];

	return true;
}

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::removeFD_(int const fd){
	selector_.removeFD(fd);

	clients_.erase(fd);
}

template<class SELECTOR, class WORKER>
void AsyncLoop<SELECTOR, WORKER>::expireFD_(){
	for(const auto &p : clients_){
		int const fd = p.first;
		auto &c = p.second;

		if (c.expired(conf_connectionTimeout_)){
			handleDisconnect_(fd, DisconnectStatus::TIMEOUT);
			// iterator is invalid now...
			return;
		}
	}
}

} // namespace

