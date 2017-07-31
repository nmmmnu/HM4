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
	constexpr static int		WAIT_TIMEOUT		= 5;
	constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;

	constexpr static size_t		BUFFER_CAPACITY 	= 1024 * 4;

private:
	constexpr static int		WAIT_TIMEOUT_MS		=  WAIT_TIMEOUT * 1000;

	using ClientBufferContainer	= std::unordered_map<int, ClientBuffer>;

	using WorkerStatus		= worker::WorkerStatus;

public:
	AsyncLoop(SELECTOR &&selector, WORKER &&worker, const std::initializer_list<int> &serverFD,
				uint32_t conf_connectionTimeout = 0,
				size_t conf_maxPacketSize = 0);
	~AsyncLoop();
	AsyncLoop(AsyncLoop &&other) = default;
	AsyncLoop &operator=(AsyncLoop &&other) = default;

	bool process();

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

	bool isServerFD_(int fd) const;

private:
	bool insertFD_(int fd);
	void removeFD_(int fd);
	void expireFD_();

private:
	void log_(const char *s, int const fd = -1) const{
		if (! LOG_ENABLED)
			return;

		if (fd < 0)
			printf("%-40s | clients: %5u |\n",         s, connectedClients_);
		else
			printf("%-40s | clients: %5u | fd: %5d\n", s, connectedClients_, fd);
	}

private:
	template<typename T>
	static T my_clamp__(const T &val, const T &min){
		// using std::max will result in link error,
		// because it returns reference
		return val > min ? val : min;
	}

private:
	SELECTOR		selector_;
	WORKER			worker_;
	std::vector<int>	serverFD_;
	ClientBufferContainer	clients_;
	uint32_t		connectedClients_ = 0;
	bool			keepProcessing_ = true;

	uint32_t		conf_connectionTimeout_;
	size_t			conf_maxPacketSize_;

	char			inputBuffer_[4 * 1024];
};


} // namespace

// ===========================

#include "asyncloop_impl.h"

#endif

