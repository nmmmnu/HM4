#ifndef _NET_ASYNC_LOOP_H
#define _NET_ASYNC_LOOP_H

#include "selector/selectordefs.h"

#include "smallvector.h"

#include "iobuffer.h"
#include "mytime.h"

#include "nullsparepool.h"

#include <unordered_map>
#include <algorithm>	// min, find

#include "logger.h"

#define FMT_HEADER_ONLY
#include "fmt/core.h"

namespace net{

template<class Selector, class Worker, class SparePool = NullSparePool>
class AsyncLoop{
public:
	constexpr static uint32_t	MIN_CLIENTS		= 4;
	constexpr static uint32_t	MAX_CLIENTS		= 32;

	constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;
	constexpr static int		WAIT_TIMEOUT		= 5;

	constexpr static size_t		IO_BUFFER_CAPACITY	= 1024 * 4;

private:
	constexpr static int		WAIT_TIMEOUT_MS		=  WAIT_TIMEOUT * 1000;

	struct Client{
		IOBuffer	buffer;
		MyTimer 	timer;

		Client(size_t conf_buffer_spare_pool) : buffer(conf_buffer_spare_pool){
		}

		Client(IOBuffer::container_type &&b) : buffer(std::move(b)){}
	};

	using ClientContainer		= std::unordered_map<int, Client>;

	using WorkerStatus		= worker::WorkerStatus;


	constexpr static const char *FMT_MASK   = "{:40} | clients: {:5} | spare_pool: {:5}"		;
	constexpr static const char *FMT_MASK_2 = "{:40} | clients: {:5} | spare_pool: {:5} | fd: {:5}"	;

public:
	AsyncLoop(Selector &&selector, Worker &&worker, const std::initializer_list<int> &serverFD,
				uint32_t conf_maxClients		= MAX_CLIENTS,
				uint32_t conf_minSparePoolSize		= MIN_CLIENTS,
				uint32_t conf_maxSparePoolSize		= MIN_CLIENTS,
				uint32_t conf_connectionTimeout		= CONNECTION_TIMEOUT,
				size_t   conf_buffer_capacity		= IO_BUFFER_CAPACITY,
				size_t   conf_maxRequestSize		= IO_BUFFER_CAPACITY
	);

	bool process();

	void idle_loop();

	auto connectedClients() const{
		return clients_.size();
	}

	auto sparePoolSize() const{
		return sparePool_.size();
	}

	void print() const;

	void printInfo(const char *msg) const{
		return log_<Logger::NOTICE>(msg);
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
	template<bool NL = true>
	bool client_Connect_(int fd);

	template<bool NL = true>
	void client_Disconnect_(int fd, DisconnectStatus error);

	void client_Read_(int fd);
	void client_Read_(int fd, std::true_type);
	void client_Read_(int fd, std::false_type);

	void client_Write_(int fd);
	void client_Write_(int fd, std::true_type);
	void client_Write_(int, std::false_type);

	bool client_Worker_(int fd, IOBuffer &buffer);

	void client_SocketOps_(int fd, ssize_t size);

	struct ConnectionNotification{
		uint64_t clients;
		uint64_t spare;
	};

	void notify_worker_(){
		worker_.connection_notify(
			ConnectionNotification{
				connectedClients(),
				sparePoolSize()
			}
		);
	}

private:
	bool insertFD_(int fd);
	void removeFD_(int fd);
	void expireFD_();

private:
	template<Logger::Level level = Logger::DEBUG>
	void log_(const char *msg) const{
		logger_fmt<level>(FMT_MASK,   msg, connectedClients(), sparePoolSize()	);
	}

	template<Logger::Level level = Logger::NOTICE>
	void log_(const char *msg, int const fd) const{
		logger_fmt<level>(FMT_MASK_2, msg, connectedClients(), sparePoolSize(), fd	);
	}

private:
	Selector		selector_;
	Worker			worker_;
	SmallVector<int, 8>	serverFD_;
	ClientContainer		clients_;
	bool			keepProcessing_ = true;

	uint32_t		conf_maxClients_;

	uint32_t		conf_minSparePoolSize_;
	uint32_t		conf_maxSparePoolSize_;

	uint32_t		conf_connectionTimeout_;

	size_t			conf_bufferCapacity_;
	size_t			conf_maxRequestSize_;

	SparePool		sparePool_{ conf_minSparePoolSize_, conf_maxSparePoolSize_, conf_bufferCapacity_ };
};


} // namespace

// ===========================

#include "asyncloop.h.cc"

#endif

