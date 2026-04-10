#include "sockets.h"
#include "mynarrow.h"

#include "worker/workerdefs.h"

#include <cassert>

namespace net{

template<class IOEngine, class Worker, class SparePool, class Storage>
AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::AsyncCompletionLoop(
			IOEngine &&ioEngine, Worker &&worker, const std::initializer_list<int> &serverFD,
			uint32_t const conf_rlimitNoFile	,
			uint32_t const conf_maxClients		,
			uint32_t const conf_minSparePoolSize	,
			uint32_t const conf_maxSparePoolSize	,
			size_t   const conf_buffer_capacity	,
			size_t   const conf_maxRequestSize
		) :
					ioEngine_		(std::move(ioEngine)	),
					worker_			(std::move(worker)	),

					conf_rlimitNoFile_	( 		(conf_rlimitNoFile								) ),
					conf_maxClients_	( std::max	(conf_maxClients,		MIN_CLIENTS					) ),
					conf_minSparePoolSize_	( std::clamp	(conf_minSparePoolSize,		MIN_CLIENTS,		conf_maxClients_	) ),
					conf_maxSparePoolSize_	( std::clamp	(conf_maxSparePoolSize,		conf_minSparePoolSize_,	conf_maxClients_	) ),
					conf_bufferCapacity_	( std::max	(conf_buffer_capacity,		IO_BUFFER_CAPACITY				) ),
					conf_maxRequestSize_	( std::max	(conf_maxRequestSize,		conf_bufferCapacity_				) ){

	assert(conf_maxClients_ < conf_rlimitNoFile_);

	for(int const fd : serverFD){
		socket_options_setNonBlocking(fd);
		ioEngine_.addServerFD(fd);
	}
}

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::print() const{
	auto _ = [](auto k, auto v){
		logger_fmt<Logger::STARTUP>("{:20} = {:12}", k, v);
	};

	_("rlimit nofile"	, conf_rlimitNoFile_		);
	_("max clients"		, conf_maxClients_		);
	_("min spare pool"	, conf_minSparePoolSize_	);
	_("max spare pool"	, conf_maxSparePoolSize_	);
	_("buffer capacity"	, conf_bufferCapacity_		);
	_("max request size"	, conf_maxRequestSize_		);
}

// ===========================

template<class IOEngine, class Worker, class SparePool, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::process(){
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

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::idle_loop(){
	expireFD_();

	sparePool_.balance();

	notify_worker_();
}

// ===========================

template<class IOEngine, class Worker, class SparePool, class Storage>
template<bool NL>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::done_Close_(int fd){
	removeFD_(fd);

	if constexpr(NL)
		notify_worker_();
}

template<class IOEngine, class Worker, class SparePool, class Storage>
template<bool NL>
bool AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::done_Accept_(int fd){
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

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::done_Read_(int fd, int result){
	Client *it = clients_[fd];

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

	if (client.buffer.size() > conf_maxRequestSize_)
		return req_Close_(fd, DisconnectStatus::PROBLEM_BUFFER_READ);

	size_t const size = static_cast<size_t>(result);

	client.buffer.finalizeWriteBuffer(client.offcet, size);

	client_Worker_(fd, client);

	// req_Read_, req_Write_, req_Close_ is sent from the client_Worker_
}

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::done_Write_(int fd, int result){
	Client *it = clients_[fd];

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
		client.buffer.clear();

		req_Read_ (fd, client);
	}else{
		req_Write_(fd, client);
	}
}

// ===========================

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::log_Close_(int const fd, const DisconnectStatus error){
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

template<class IOEngine, class Worker, class SparePool, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::client_Worker_(int const fd, Client &client){
	const WorkerStatus status = worker_( client.buffer );

	#if 0
		switch(status){
		case WorkerStatus::PASS		: printf("PASS		\n"); break;
		case WorkerStatus::READ		: printf("READ		\n"); break;
		case WorkerStatus::WRITE	: printf("WRITE		\n"); break;
		default				: printf("<other>	\n"); break;
		}
	#endif

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

template<class IOEngine, class Worker, class SparePool, class Storage>
bool AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::insertFD_(int const fd){
	if ( !clients_.insert(fd, sparePool_.pop()) )
		return false;

	// this is guaranteed not null,
	// because we just inserted it.
	Client *it = clients_[fd];

	auto &client = *it;

	req_Read_(fd, client);

	return true;
}

template<class IOEngine, class Worker, class SparePool, class Storage>
void AsyncCompletionLoop<IOEngine, Worker, SparePool, Storage>::removeFD_(int const fd){
	// Find and steal

	Client *it = clients_[fd];

	if (!it){
		// do nothing, error is already reported.
		return;
	}

	IOBuffer &buffer = it->buffer;

	// steal
	sparePool_.push(std::move(buffer.getBuffer()));

	clients_.remove(fd);
}

} // namespace

