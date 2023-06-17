#include "sockets.h"
#include "mynarrow.h"

#include "worker/workerdefs.h"

#include <unistd.h>	// read

namespace net{

namespace impl_{
	template<class Container>
	bool findFD(Container const &c, int const fd){
		return std::find(std::begin(c), std::end(c), fd) != std::end(c);
	}
}

template<class Selector, class Worker, class SparePool>
AsyncLoop<Selector, Worker, SparePool>::AsyncLoop(
			Selector &&selector, Worker &&worker, const std::initializer_list<int> &serverFD,
			uint32_t const conf_maxClients		,
			uint32_t const conf_minSparePoolSize	,
			uint32_t const conf_maxSparePoolSize	,
			uint32_t const conf_connectionTimeout	,
			size_t   const conf_buffer_capacity	,
			size_t   const conf_maxRequestSize
		) :
					selector_		(std::move(selector)	),
					worker_			(std::move(worker)	),
					serverFD_		(serverFD		),

					conf_maxClients_	( std::max	(conf_maxClients,		MIN_CLIENTS					) ),
					conf_minSparePoolSize_	( std::clamp	(conf_minSparePoolSize,		MIN_CLIENTS,		conf_maxClients_	) ),
					conf_maxSparePoolSize_	( std::clamp	(conf_maxSparePoolSize,		conf_minSparePoolSize_,	conf_maxClients_	) ),
					conf_connectionTimeout_	( 		(conf_connectionTimeout								) ),
					conf_bufferCapacity_	( std::max	(conf_buffer_capacity,		IO_BUFFER_CAPACITY				) ),
					conf_maxRequestSize_	( std::max	(conf_maxRequestSize,		conf_bufferCapacity_				) ){

	// fixParameters();

	for(int const fd : serverFD_){
		socket_options_setNonBlocking(fd);
		selector_.insertFD(fd);
	}
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::print() const{
	auto _ = [](auto k, auto v){
		logger_fmt<Logger::STARTUP>("{:20} = {:12}", k, v);
	};

	_("max clients"		, conf_maxClients_		);
	_("min spare pool"	, conf_minSparePoolSize_	);
	_("max spare pool"	, conf_maxSparePoolSize_	);
	_("timeout"		, conf_connectionTimeout_	);
	_("buffer capacity"	, conf_bufferCapacity_		);
	_("max request size"	, conf_maxRequestSize_		);
}

// ===========================

template<class Selector, class Worker, class SparePool>
bool AsyncLoop<Selector, Worker, SparePool>::process(){
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
		idle_loop();
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

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::idle_loop(){
	expireFD_();

	sparePool_.balance();

	notify_worker_();
}

// ===========================

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Read_(int fd){
	if ( impl_::findFD(serverFD_, fd) )
		client_Read_(fd, std::false_type{});
	else
		client_Read_(fd, std::true_type{});
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Read_(int const fd, std::false_type){
	while (client_Connect_(fd));
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Read_(int const fd, std::true_type){
	auto it = clients_.find(fd);

	if (it == clients_.end())
		return client_Disconnect_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &client = it->second;

	// -------------------------------------

	if constexpr(false){
		char buffer[IO_BUFFER_CAPACITY];

		ssize_t const size = ::read(fd, buffer, IO_BUFFER_CAPACITY);

		if (size <= 0)
			return client_SocketOps_(fd, size);

		if (client.buffer.size() + narrow<size_t>(size) > conf_maxRequestSize_)
			return client_Disconnect_(fd, DisconnectStatus::ERROR);

		// size is checked already
		client.buffer.push( static_cast<size_t>(size), buffer);
	}else{
		ssize_t size;

		client.buffer.push(std::true_type{}, IO_BUFFER_CAPACITY, [&size, fd](void *buffer){
			size = ::read(fd, buffer, IO_BUFFER_CAPACITY);

			return size;
		});

		if (size <= 0)
			return client_SocketOps_(fd, size);

		if (client.buffer.size() > conf_maxRequestSize_)
			return client_Disconnect_(fd, DisconnectStatus::ERROR);
	}

	if constexpr(false){
		client.buffer.print(false);
	}

	client.timer.restart();

	client_Worker_(fd, client.buffer);
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Write_(int fd){
if ( impl_::findFD(serverFD_, fd) )
	client_Write_(fd, std::false_type{});
else
	client_Write_(fd, std::true_type{});
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Write_(int, std::false_type){
	// WTF?!? Should never happen...
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_Write_(int const fd, std::true_type){
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

template<class Selector, class Worker, class SparePool>
bool AsyncLoop<Selector, Worker, SparePool>::client_Worker_(int const fd, IOBuffer &buffer){
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

template<class Selector, class Worker, class SparePool>
template<bool NL>
bool AsyncLoop<Selector, Worker, SparePool>::client_Connect_(int const fd){
	// fd is same as serverFD_
	int const newFD = socket_accept(fd);

	// serverFD_ is non blocking, so we do not need to check EAGAIN
	if (newFD < 0)
		return false;

	if ( clients_.size() < conf_maxClients_ && insertFD_(newFD) ){
		// socket_options_setNonBlocking(newFD);

		if constexpr(NL)
			notify_worker_();

		log_("Connect", newFD);
	}else{
		socket_close(newFD);

		log_("Drop connect", newFD);
	}

	return true;
}

template<class Selector, class Worker, class SparePool>
template<bool NL>
void AsyncLoop<Selector, Worker, SparePool>::client_Disconnect_(int const fd, const DisconnectStatus error){
	removeFD_(fd);

	socket_close(fd);

	if constexpr(NL)
		notify_worker_();

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

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::client_SocketOps_(int const fd, ssize_t const size){
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

template<class Selector, class Worker, class SparePool>
bool AsyncLoop<Selector, Worker, SparePool>::insertFD_(int const fd){
	if ( ! selector_.insertFD(fd) )
		return false;

	clients_.emplace(fd, sparePool_.pop() );

	return true;
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::removeFD_(int const fd){
	selector_.removeFD(fd);

	// Find and steal

	auto it = clients_.find(fd);

	if (it == clients_.end()){
		// do nothing, error is already reported.
		return;
	}

	IOBuffer &buffer = it->second.buffer;

	// steal
	sparePool_.push(std::move(buffer.getBuffer()));

	// erase
	clients_.erase(it);
}

template<class Selector, class Worker, class SparePool>
void AsyncLoop<Selector, Worker, SparePool>::expireFD_(){
	for(const auto &[fd, c] : clients_){
		if (c.timer.expired(conf_connectionTimeout_)){
			client_Disconnect_<0>(fd, DisconnectStatus::TIMEOUT);
			// iterator is invalid now...
			return;
		}
	}
}

} // namespace

