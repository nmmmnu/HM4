#include "sockets.h"
#include "mynarrow.h"

#include "worker/workerdefs.h"

#include <cassert>

namespace net{

template<class IOEngine, class Worker, class MemoryManager, class Storage>
AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::AsyncCompletionLoop(
			IOEngine &&ioEngine, Worker &&worker, const std::initializer_list<int> &serverFD, MemoryManager &mm,
			uint32_t const conf_rlimitNoFile	,
			uint32_t const conf_maxClients		,
		//	size_t   const conf_buffer_capacity	,
			size_t   const conf_maxRequestSize
		) :
					ioEngine_		(std::move(ioEngine)	),
					worker_			(std::move(worker)	),
					mm_			(mm			),

					conf_rlimitNoFile_	( 		(conf_rlimitNoFile				) ),
					conf_maxClients_	( std::max	(conf_maxClients,	MIN_CLIENTS		) ),
				//	conf_bufferCapacity_	( std::max	(conf_buffer_capacity,	IO_BUFFER_CAPACITY	) ),
					conf_maxRequestSize_	( std::max	(conf_maxRequestSize,	conf_bufferCapacity_	) ){

	assert(conf_maxClients_ < conf_rlimitNoFile_);

	for(int const fd : serverFD){
		socket_options_setNonBlocking(fd);
		ioEngine_.addServerFD(fd);
	}
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::print() const{
	auto _ = [](auto k, auto v){
		logger_fmt<Logger::STARTUP>("{:25} = {:12}", k, v);
	};

	_("rlimit nofile"	, conf_rlimitNoFile_		);
	_("max clients"		, conf_maxClients_		);
	_("buffer capacity"	, conf_bufferCapacity_		);
	_("max request size"	, conf_maxRequestSize_		);
}

// ===========================

template<class IOEngine, class Worker, class MemoryManager, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::process(){
	using WaitStatus  = ioengine::WaitStatus;
	using FDOperation = ioengine::FDOperation;

	keepProcessing_ = true;

	log_("poll()-ing...");
	WaitStatus const status = ioEngine_.wait(true);

	if (status == WaitStatus::ERROR){
		log_("poll() error");

		return false;
	}

	if (status == WaitStatus::NONE){
		// idle loop, check for expired conn
		idle_loop();
		return true;
	}

	for(auto const t : ioEngine_){
		switch(t.op){
		case FDOperation::ACCEPT:
			done_Accept_(t.result);
			break;

		case FDOperation::READ:
			done_Read_(t.fd, t.result);
			break;

		case FDOperation::WRITE:
			done_Write_(t.fd, t.result);
			break;

		case FDOperation::CLOSE:
			done_Close_(t.fd);
			break;

		case FDOperation::NONE:
		default:
			break;

		}
	}

	return keepProcessing_;
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::idle_loop(){
	expireFD_();

	notify_worker_();
}

// ===========================

template<class IOEngine, class Worker, class MemoryManager, class Storage>
template<bool NL>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::done_Close_(int fd){
	removeFD_(fd);

	if constexpr(NL)
		notify_worker_();
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
template<bool NL>
bool AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::done_Accept_(int fd){
	if (fd < 0)
		return false;

	if ( clients_.size() < conf_maxClients_ && insertFD_(fd) ){
		// socket_options_setNonBlocking(newFD);

		if constexpr(NL)
			notify_worker_();

		log_("Connect", fd);
	}else{
		socket_close(fd);

		log_("Drop connect", fd);
	}

	return true;
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::done_Read_(int fd, int result){
	auto *it = clients_[fd];

	if (!it)
		return req_Close_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &client = *it;

	// -------------------------------------

	if (result == -ECANCELED)
		return req_Close_(fd, DisconnectStatus::TIMEOUT);

	if (result < 0)
		return req_Close_(fd, DisconnectStatus::ERROR);

	if (result == 0)
		return req_Close_(fd, DisconnectStatus::NORMAL);

	// -------------------------------------

	size_t const size = static_cast<size_t>(result);

	client.buffer.finalizeBuffer(client.offcet, size);

	if (client.buffer.size() > conf_maxRequestSize_)
		return req_Close_(fd, DisconnectStatus::PROBLEM_BUFFER_READ);

	client_Worker_(fd, client);

	// req_Read_, req_Write_, req_Close_ is sent from the client_Worker_
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::done_Write_(int fd, int result){
	auto *it = clients_[fd];

	if (!it)
		return req_Close_(fd, DisconnectStatus::PROBLEM_MAP_NOT_FOUND);

	auto &client = *it;

	// -------------------------------------

	if (result == -ECANCELED)
		return req_Close_(fd, DisconnectStatus::TIMEOUT);

	if (result < 0)
		return req_Close_(fd, DisconnectStatus::ERROR);

	if (result == 0)
		return req_Close_(fd, DisconnectStatus::NORMAL);

	// -------------------------------------

	if (client.buffer.size() > 0){
		size_t const size = static_cast<size_t>(result);

		client.buffer.pop(size);
	}

	if (client.buffer.size() == 0){
		// no need req_FreshRead_, size is zero already
		req_Read_(fd, client);
	}else{
		req_Write_(fd, client);
	}
}

// ===========================

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::log_Close_(int const fd, const DisconnectStatus error){
	switch(error){
	case DisconnectStatus::NORMAL			: return log_("Request Normal  Disconnect",				fd);
	case DisconnectStatus::ERROR			: return log_("Request Error   Disconnect",				fd);
	case DisconnectStatus::TIMEOUT			: return log_("Request Timeout Disconnect",				fd);

	case DisconnectStatus::Worker_NORMAL		: return log_("Request Worker  Disconnect - Normal",			fd);
	case DisconnectStatus::Worker_ERROR		: return log_("Request Worker  Disconnect - Error",			fd);

	case DisconnectStatus::PROBLEM_MAP_NOT_FOUND	: return log_("Request Problem Disconnect - FD not found",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_READ	: return log_("Request Problem Disconnect - Read buffer full",		fd);
	case DisconnectStatus::PROBLEM_BUFFER_WRITE	: return log_("Request Problem Disconnect - Write buffer problem",	fd);
	};
}

// ===========================

template<class IOEngine, class Worker, class MemoryManager, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::client_Worker_(int const fd, Client<MemoryManager> &client){
	const WorkerStatus status = worker_( client.buffer );

	// buffer is cleared unless PASS

	switch(status){
	case WorkerStatus::PASS:
		req_Read_(fd, client);

		return false;

	case WorkerStatus::READ:
		req_Read_(fd, client);

		return true;

	case WorkerStatus::WRITE:
		req_Write_(fd, client);

		return true;

	case WorkerStatus::DISCONNECT:
		req_Close_(fd, DisconnectStatus::Worker_NORMAL);

		return true;

	case WorkerStatus::SHUTDOWN:
	//	client_Disconnect_(fd, DisconnectStatus::Worker_NORMAL);
		keepProcessing_ = false;

		return true;

	default:
	case WorkerStatus::DISCONNECT_ERROR:
		req_Close_(fd, DisconnectStatus::Worker_ERROR);

		return true;
	}
}

// ===========================

template<class IOEngine, class Worker, class MemoryManager, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::insertFD_(int const fd){
	if ( !clients_.insert(fd, mm_) )
		return false;

	// this is guaranteed not null,
	// because we just inserted it.
	auto *it = clients_[fd];

	auto &client = *it;

	// no need req_FreshRead_, size is zero already
	req_Read_(fd, client);

	return true;
}

template<class IOEngine, class Worker, class MemoryManager, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, MemoryManager, Storage>::removeFD_(int const fd){
	auto *it = clients_[fd];

	if (!it){
		// do nothing, error is already removed.
		return;
	}

	clients_.remove(fd);
}

} // namespace

