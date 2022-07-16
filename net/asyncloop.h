#ifndef _NET_ASYNC_LOOP_H
#define _NET_ASYNC_LOOP_H

#include "selector/selectordefs.h"

#include "iobuffer.h"
#include "mytime.h"

#include <unordered_map>

namespace net{

template<class Selector, class Worker>
class AsyncLoop{
public:
	constexpr static bool		LOG_ENABLED		= true;

public:
	constexpr static uint32_t	MIN_CLIENTS		= 4;
	constexpr static uint32_t	MAX_CLIENTS		= 32;
	constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;
	constexpr static int		WAIT_TIMEOUT		= 5;

	constexpr static size_t		BUFFER_CAPACITY 	= 1024 * 4;

private:
	constexpr static int		WAIT_TIMEOUT_MS		=  WAIT_TIMEOUT * 1000;

	struct Client{
		IOBuffer	buffer;
		MyTimer 	timer;
	};

	using ClientContainer	= std::unordered_map<int, Client>;

	using WorkerStatus	= worker::WorkerStatus;

public:
	AsyncLoop(Selector &&selector, Worker &&worker, const std::initializer_list<int> &serverFD,
				uint32_t conf_maxClients	= MAX_CLIENTS,
				uint32_t conf_connectionTimeout	= 0,
				size_t   conf_maxRequestSize	= 0
	);

	bool process();

	auto connectedClients() const{
		return clients_.size();
	}

private:
	enum class DisconnectStatus{
		NORMAL,
		ERROR,
		TIMEOUT,

		Worker_NORMAL,
		Worker_ERROR,

		PROBLEM_MAP_NOT_FOUND,
		PROBLEM_BUFFER_READ,
		PROBLEM_BUFFER_WRITE
	};

private:
	void handleRead_(int fd);
	void handleWrite_(int fd);
	bool handleConnect_(int fd);
	void handleDisconnect_(int fd, const DisconnectStatus error);
	bool handleWorker_(int fd, IOBuffer &buffer);

	void handleSocketOps_(int fd, ssize_t size);

private:
	bool insertFD_(int fd);
	void removeFD_(int fd);
	void expireFD_();

private:
	void log_(const char *s, int const fd = -1) const{
		if (! LOG_ENABLED)
			return;

		// printf suppose to be faster than std::cout

		if (fd < 0)
			fprintf(stderr, "%-40s | clients: %5zu |\n",         s, connectedClients());
		else
			fprintf(stderr, "%-40s | clients: %5zu | fd: %5d\n", s, connectedClients(), fd);
	}

private:
	Selector		selector_;
	Worker			worker_;
	std::vector<int>	serverFD_;
	ClientContainer		clients_;
	bool			keepProcessing_ = true;

	uint32_t		conf_maxClients;
	uint32_t		conf_connectionTimeout_;
	size_t			conf_maxRequestSize_;

	char			inputBuffer_[BUFFER_CAPACITY];
};


} // namespace

// ===========================

#include "asyncloop.h.cc"

#endif

