#ifndef _NET_ASYNC_LOOP_H
#define _NET_ASYNC_LOOP_H

#include "selector/selectordefs.h"
#include "clientbuffer.h"

#include <unordered_map>

namespace net{

template<class SELECTOR, class WORKER>
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

	using ClientBufferContainer	= std::unordered_map<int, ClientBuffer>;

	using WorkerStatus		= worker::WorkerStatus;

public:
	AsyncLoop(SELECTOR &&selector, WORKER &&worker, const std::initializer_list<int> &serverFD,
				uint32_t conf_maxClients	= MAX_CLIENTS,
				uint32_t conf_connectionTimeout	= 0,
				size_t   conf_maxPacketSize	= 0
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

		WORKER_NORMAL,
		WORKER_ERROR,

		PROBLEM_MAP_NOT_FOUND,
		PROBLEM_BUFFER_READ,
		PROBLEM_BUFFER_WRITE
	};

private:
	void handleRead_(int fd);
	void handleWrite_(int fd);
	bool handleConnect_(int fd);
	void handleDisconnect_(int fd, const DisconnectStatus error);
	bool handleWorker_(int fd, ClientBuffer &connection);

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
	template<typename T>
	constexpr static T max__(T const a, T const b){
		// using std::max will result in link error,
		// because it returns reference
		return a > b ? a : b;
	}

private:
	SELECTOR		selector_;
	WORKER			worker_;
	std::vector<int>	serverFD_;
	ClientBufferContainer	clients_;
	bool			keepProcessing_ = true;

	uint32_t		conf_maxClients;
	uint32_t		conf_connectionTimeout_;
	size_t			conf_maxPacketSize_;

	char			inputBuffer_[4 * 1024];
};


} // namespace

// ===========================

#include "asyncloop.h.cc"

#endif

