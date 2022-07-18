#include "sockets.h"
#include "mynarrow.h"

#include "worker/workerdefs.h"

#include <unistd.h>	// read
#include <algorithm>	// find
#include <iostream>

namespace net{

namespace impl_{
	template<class Container>
	bool findFD(Container const &c, int const fd){
		return std::find(std::begin(c), std::end(c), fd) != std::end(c);
	}

#if 0
	template<typename T>
	constexpr static T max(T const a, T const b){
		// using std::max directly will result in link error,
		// because it returns reference
		return std::max(a, b);
	}
#endif
}

template<class Selector, class Worker>
AsyncLoop<Selector, Worker>::AsyncLoop(Selector &&selector, Worker &&worker, const std::initializer_list<int> &serverFD,
			uint32_t const conf_maxClients		,
			uint32_t const conf_connectionTimeout	,
			size_t   const conf_maxRequestSize
		) :
					selector_(std::move(selector)),
					worker_(std::move(worker)),
					serverFD_(serverFD),

					conf_maxClients		(	std::max(conf_maxClients,		MIN_CLIENTS		)),
					conf_connectionTimeout_	(	std::max(conf_connectionTimeout,	CONNECTION_TIMEOUT	)),
					conf_maxRequestSize_	(	std::max(conf_maxRequestSize,		BUFFER_CAPACITY		)){
	for(int const fd : serverFD_){
		socket_options_setNonBlocking(fd);
		selector_.insertFD(fd);
	}
}

// ===========================

template<class Selector, class Worker>
bool AsyncLoop<Selector, Worker>::process(){
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

	for(auto const t : selector_){
		switch(t.status){
		case FDStatus::READ:
			client_Read_( t.fd );
			break;

		case FDStatus::WRITE:
			client_Write_( t.fd );
			break;

		case FDStatus::ERROR:
			client_Disconnect_( t.fd, DisconnectStatus::ERROR );
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

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Read_(int fd){
	if ( impl_::findFD(serverFD_, fd) )
		client_Read_(fd, std::false_type{});
	else
		client_Read_(fd, std::true_type{});
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Read_(int const fd, std::false_type){
	while (client_Connect_(fd));
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Read_(int const fd, std::true_type){
	auto it = clients_.find(fd);

	if (it == clients_.end())
		return client_Disconnect_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &client = it->second;

	// -------------------------------------

	ssize_t const size = ::read(fd, inputBuffer_, BUFFER_CAPACITY);

	if (size <= 0)
		return client_SocketOps_(fd, size);

	if (client.buffer.size() + narrow<size_t>(size) > conf_maxRequestSize_)
		return client_Disconnect_(fd, DisconnectStatus::ERROR);

	// size is checked already
	client.buffer.push( static_cast<size_t>(size), inputBuffer_);

	client.timer.restart();

	client_Worker_(fd, client.buffer);
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Write_(int fd){
if ( impl_::findFD(serverFD_, fd) )
	client_Write_(fd, std::false_type{});
else
	client_Write_(fd, std::true_type{});
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Write_(int, std::false_type){
	// WTF?!? Should never happen...
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Write_(int const fd, std::true_type){
	auto it = clients_.find(fd);

	if (it == clients_.end())
		return client_Disconnect_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &client = it->second;

	// -------------------------------------

	if (client.buffer.size() > 0){
		// available == 0 - this might happen on MacOS with kqueue()

		ssize_t const size = ::write(fd, client.buffer.data(), client.buffer.size());

		if (size <= 0)
			return client_SocketOps_(fd, size);

		// size is checked already
		client.buffer.pop( static_cast<size_t>(size) );

		client.timer.restart();
	}

	if (client.buffer.size() == 0){
		// process with read
		selector_.updateFD(fd, selector::FDEvent::READ);

		return;
	}
}

template<class Selector, class Worker>
bool AsyncLoop<Selector, Worker>::client_Worker_(int const fd, IOBuffer &buffer){
	const WorkerStatus status = worker_( buffer );

	switch(status){
	case WorkerStatus::PASS:
		return false;

	case WorkerStatus::READ:
		selector_.updateFD(fd, selector::FDEvent::READ);

		return true;

	case WorkerStatus::WRITE:
		selector_.updateFD(fd, selector::FDEvent::WRITE);

		return true;

	case WorkerStatus::DISCONNECT:
		client_Disconnect_(fd, DisconnectStatus::Worker_NORMAL);

		return true;

	case WorkerStatus::SHUTDOWN:
	//	client_Disconnect_(fd, DisconnectStatus::Worker_NORMAL);
		keepProcessing_ = false;

		return true;

	default:
	case WorkerStatus::DISCONNECT_ERROR:
		client_Disconnect_(fd, DisconnectStatus::Worker_ERROR);

		return true;
	}
}

template<class Selector, class Worker>
bool AsyncLoop<Selector, Worker>::client_Connect_(int const fd){
	// fd is same as serverFD_
	int const newFD = socket_accept(fd);

	// serverFD_ is non blocking, so we do not need to check EAGAIN
	if (newFD < 0)
		return false;

	if ( clients_.size() < conf_maxClients && iinsertFD_(newFD) ){
		// socket_options_setNonBlocking(newFD);

		log_("Connect", newFD);
	}else{
		socket_close(newFD);

		log_("Drop connect", newFD);
	}

	return true;
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_Disconnect_(int const fd, const DisconnectStatus error){
	removeFD_(fd);

	socket_close(fd);

	switch(error){
	case DisconnectStatus::NORMAL			: return log_("Normal  Disconnect",				fd);
	case DisconnectStatus::ERROR			: return log_("Error   Disconnect",				fd);
	case DisconnectStatus::TIMEOUT			: return log_("Timeout Disconnect",				fd);

	case DisconnectStatus::Worker_NORMAL		: return log_("Worker  Disconnect - Normal",			fd);
	case DisconnectStatus::Worker_ERROR		: return log_("Worker  Disconnect - Error",			fd);

	case DisconnectStatus::PROBLEM_MAP_NOT_FOUND	: return log_("Problem Disconnect - FD not found",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_READ	: return log_("Problem Disconnect - Read buffer full",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_WRITE	: return log_("Problem Disconnect - Write buffer problem",	fd);
	};
}

// ===========================

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::client_SocketOps_(int const fd, ssize_t const size){
	if (size < 0){
		if ( socket_check_eagain() ){
			// try again
			return;
		}else{
			// error, disconnect.
			return client_Disconnect_(fd, DisconnectStatus::ERROR);
		}
	}

	if (size == 0){
		// normal, disconnect.
		return client_Disconnect_(fd, DisconnectStatus::NORMAL);
	}
}

// ===========================

template<class Selector, class Worker>
bool AsyncLoop<Selector, Worker>::iinsertFD_(int const fd){
	if ( ! selector_.insertFD(fd) )
		return false;

	clients_[fd];

	return true;
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::removeFD_(int const fd){
	selector_.removeFD(fd);

	clients_.erase(fd);
}

template<class Selector, class Worker>
void AsyncLoop<Selector, Worker>::expireFD_(){
	for(const auto &[fd, c] : clients_){
		if (c.timer.expired(conf_connectionTimeout_)){
			client_Disconnect_(fd, DisconnectStatus::TIMEOUT);
			// iterator is invalid now...
			return;
		}
	}
}

} // namespace

