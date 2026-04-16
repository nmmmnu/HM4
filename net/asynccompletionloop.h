#ifndef _NET_ASYNC_COMPLETION_LOOP_H
#define _NET_ASYNC_COMPLETION_LOOP_H

#include "ioengine/ioenginedefs.h"
#include "worker/workerdefs.h"

#include "smallvector.h"

#include "asynccompletionloop.client.h"

#include "nullsparepool.h"
//#include "asyncloop.dynamicarrayfdstorage.h"
//#include "asyncloop.slabfdstorage.h"
#include "asyncloop.sparsefdstorage.h"

#include <algorithm>	// min, find

#include "logger.h"

#define FMT_HEADER_ONLY
#include "fmt/core.h"

namespace net{
//	using MyFDStorage = DynamicArrayFDStorage<Client>;
//	using MyFDStorage = SlabFDStorage<Client>;
	using MyFDStorage = SparseFDStorage<Client>;

	template<class IOEngine, class Worker, class SparePool = NullSparePool, class FDStorage = MyFDStorage>
	class AsyncCompletionLoop{
	public:
		constexpr static uint32_t	MIN_CLIENTS		= 4;
		constexpr static uint32_t	MAX_CLIENTS		= 32;
		constexpr static uint32_t	LIMIT_NO_FILES		= 1024;

		constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;

		constexpr static size_t		IO_BUFFER_CAPACITY	= 1024 * 4;
		constexpr static size_t		MAX_REQUEST_SIZE	= IO_BUFFER_CAPACITY * 4;

	private:
		constexpr static int32_t	WAIT_TIMEOUT		= 5;
		constexpr static int32_t	READ_TIMEOUT		= 2 * 60;
		constexpr static int32_t	WRITE_TIMEOUT		= 2 * 60;

		using WorkerStatus		= worker::WorkerStatus;


		constexpr static const char *FMT_MASK   = "{:40} | clients: {:5} | spare_pool: {:5}"		;
		constexpr static const char *FMT_MASK_2 = "{:40} | clients: {:5} | spare_pool: {:5} | fd: {:5}"	;

	public:
		AsyncCompletionLoop(IOEngine &&ioEngine, Worker &&worker, const std::initializer_list<int> &serverFD,
					uint32_t conf_rlimitNoFile	= LIMIT_NO_FILES	,
					uint32_t conf_maxClients	= MAX_CLIENTS		,
					uint32_t conf_minSparePoolSize	= MIN_CLIENTS		,
					uint32_t conf_maxSparePoolSize	= MIN_CLIENTS		,
					size_t   conf_buffer_capacity	= IO_BUFFER_CAPACITY	,
					size_t   conf_maxRequestSize	= MAX_REQUEST_SIZE
		);

		[[nodiscard]]
		bool process();

		void idle_loop();

		[[nodiscard]]
		auto connectedClients() const{
			return clients_.size();
		}

		[[nodiscard]]
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
		bool done_Accept_(int fd);

		void done_Read_  (int fd, int result);
		void done_Write_ (int fd, int result);

		template<bool NL = true>
		void done_Close_(int fd);

	private:
		void req_Read_   (int fd, Client &client, size_t size = IO_BUFFER_CAPACITY){
			auto [p, sizeNew] = client.buffer.provideWriteBufferAtLeast(size);

			client.offcet = p;

			ioEngine_.add_read(fd, p, static_cast<uint32_t>(sizeNew), true);
		}

		void req_Write_  (int fd, Client &client){
			ioEngine_.add_write(fd, client.buffer.data(), static_cast<uint32_t>(client.buffer.size()), true);
		}

		void req_Close_  (int fd, DisconnectStatus error){
			ioEngine_.add_close(fd);

			log_Close_(fd, error);
		}

	private:
		void log_Close_  (int fd, DisconnectStatus error);

	private:
		bool client_Worker_(int fd, Client &client);

		void notify_worker_(){
			struct ConnectionNotification{
				uint64_t clients;
				uint64_t spare;
			};

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

		constexpr static
		void expireFD_(){};

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
		IOEngine		ioEngine_;
		Worker			worker_;

		bool			keepProcessing_		= true;

		uint32_t		conf_rlimitNoFile_	;
		uint32_t		conf_maxClients_	;

		uint32_t		conf_minSparePoolSize_	;
		uint32_t		conf_maxSparePoolSize_	;

		size_t			conf_bufferCapacity_	;
		size_t			conf_maxRequestSize_	;

		SparePool		sparePool_{ conf_minSparePoolSize_, conf_maxSparePoolSize_, conf_bufferCapacity_ };

		FDStorage		clients_{ conf_rlimitNoFile_, conf_maxClients_ };
	};

} // namespace net

// ===========================

#include "asynccompletionloop.h.cc"

#endif

