#ifndef _NET_ASYNC_LOOP_H
#define _NET_ASYNC_LOOP_H

#include "selector/selectordefs.h"
#include "worker/workerdefs.h"

#include "smallvector.h"

#include "asyncloop.client.h"

//#include "asyncloop.dynamicarrayfdstorage.h"
//#include "asyncloop.slabfdstorage.h"
#include "asyncloop.sparsefdstorage.h"

#include <algorithm>	// min, find

#include "logger.h"



namespace net{
//	using MyFDStorage = DynamicArrayFDStorage<Client>;
//	using MyFDStorage = SlabFDStorage<Client>;
	template<typename MemoryManager>
	using MyFDStorage = SparseFDStorage<Client<MemoryManager> >;

	template<class Selector, class Worker, class MemoryManager, class FDStorage = MyFDStorage<MemoryManager> >
	class AsyncLoop{
		constexpr static uint32_t	KB			= 1024;

	public:
		constexpr static uint32_t	MIN_CLIENTS		= 4;
		constexpr static uint32_t	MAX_CLIENTS		= 32;
		constexpr static uint32_t	LIMIT_NO_FILES		= 1024;

		constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;
		constexpr static int		WAIT_TIMEOUT		= 5;

		constexpr static size_t		MAX_REQUEST_SIZE	= 64 * KB;

	private:
		constexpr static int		WAIT_TIMEOUT_MS		=  WAIT_TIMEOUT * 1000;

		using WorkerStatus		= worker::WorkerStatus;

		using MyClient			= Client<MemoryManager>;


		constexpr static auto FMT_MASK   = FMT_COMPILE("{:40} | clients: {:5}"			);
		constexpr static auto FMT_MASK_2 = FMT_COMPILE("{:40} | clients: {:5} | fd: {:5}"	);

	public:
		AsyncLoop(Selector &&selector, Worker &&worker, const std::initializer_list<int> &serverFD, MemoryManager &mm,
						uint32_t conf_rlimitNoFile	= LIMIT_NO_FILES	,
						uint32_t conf_maxClients	= MAX_CLIENTS		,
						uint32_t conf_minSparePoolSize	= MIN_CLIENTS		,
						uint32_t conf_maxSparePoolSize	= MIN_CLIENTS		,
						uint32_t conf_connectionTimeout	= CONNECTION_TIMEOUT	,
						size_t   conf_maxRequestSize	= MAX_REQUEST_SIZE
		);

		//~AsyncLoop();

		bool process();

		void idle_loop();

		auto connectedClients() const{
			return clients_.size();
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
		void client_Write_(int,    std::false_type);

		bool client_Worker_(int fd, IOBuffer<MemoryManager> &buffer);

		void client_SocketOps_(int fd, ssize_t size);

		void notify_worker_(){
			struct ConnectionNotification{
				uint64_t clients;
			};

			worker_.connection_notify(
				ConnectionNotification{
					connectedClients()
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
			logger_fmt<level>(FMT_MASK,   msg, connectedClients()	);
		}

		template<Logger::Level level = Logger::NOTICE>
		void log_(const char *msg, int const fd) const{
			logger_fmt<level>(FMT_MASK_2, msg, connectedClients(), fd	);
		}

	private:
		bool isServerFD_(int const fd) const{
			return std::find(std::begin(serverFD_), std::end(serverFD_), fd) != std::end(serverFD_);
		}

	private:
		Selector		selector_;
		Worker			worker_;
		SmallVector<int, 8>	serverFD_;

		MemoryManager		&mm_;

		bool			keepProcessing_		= true;

		uint32_t		conf_rlimitNoFile_	;
		uint32_t		conf_maxClients_	;

		uint32_t		conf_minSparePoolSize_	;
		uint32_t		conf_maxSparePoolSize_	;

		uint32_t		conf_connectionTimeout_	;

		size_t			conf_bufferCapacity_	;
		size_t			conf_maxRequestSize_	;

		FDStorage		clients_{ conf_rlimitNoFile_, conf_maxClients_ };
	};

} // namespace net

// ===========================

#include "asyncloop.h.cc"

#endif

