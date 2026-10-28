#ifndef _NET_ASYNC_COMPLETION_LOOP_H
#define _NET_ASYNC_COMPLETION_LOOP_H

#include "ioengine/ioenginedefs.h"
#include "worker/workerdefs.h"

#include "smallvector.h"

#include "asynccompletionloop.client.h"

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

	template<class IOEngine, class Worker, class MemoryManager, class FDStorage = MyFDStorage<MemoryManager> >
	class AsyncCompletionLoop{
		constexpr static uint32_t	KB			= 1024;

	public:
		constexpr static uint32_t	MIN_CLIENTS		= 4;
		constexpr static uint32_t	MAX_CLIENTS		= 32;
		constexpr static uint32_t	LIMIT_NO_FILES		= 1024;

		constexpr static uint32_t	CONNECTION_TIMEOUT	= 20;

	//	constexpr static size_t		IO_BUFFER_CAPACITY	= KB * 4;
		constexpr static size_t		MAX_REQUEST_SIZE	= 64 * 4;

	private:
		constexpr static int32_t	WAIT_TIMEOUT		= 5;
		constexpr static int32_t	READ_TIMEOUT		= 2 * 60;
		constexpr static int32_t	WRITE_TIMEOUT		= 2 * 60;

		using WorkerStatus		= worker::WorkerStatus;


		constexpr static auto FMT_MASK   = FMT_COMPILE("{:40} | clients: {:5}"			);
		constexpr static auto FMT_MASK_2 = FMT_COMPILE("{:40} | clients: {:5} | fd: {:5}"	);

	public:
		AsyncCompletionLoop(IOEngine &&ioEngine, Worker &&worker, const std::initializer_list<int> &serverFD, MemoryManager &mm,
					uint32_t conf_rlimitNoFile	= LIMIT_NO_FILES	,
					uint32_t conf_maxClients	= MAX_CLIENTS		,
				//	size_t   conf_buffer_capacity	= IO_BUFFER_CAPACITY	,
					size_t   conf_maxRequestSize	= MAX_REQUEST_SIZE
		);

		[[nodiscard]]
		bool process();

		void idle_loop();

		[[nodiscard]]
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
		bool done_Accept_(int fd);

		void done_Read_  (int fd, int result);
		void done_Write_ (int fd, int result);

		template<bool NL = true>
		void done_Close_(int fd);

	private:
		void req_Read_   (int fd, Client<MemoryManager> &client){
			auto [p, sizeNew] = client.buffer.provideBufferAtLeastStandardSize();

			client.offcet = p;

			ioEngine_.add_read(fd, p, static_cast<uint32_t>(sizeNew), true);
		}

		void req_Write_  (int fd, Client<MemoryManager> &client){
			ioEngine_.add_write(fd, client.buffer.data(), static_cast<uint32_t>(client.buffer.size()), true);
		}

		void req_Close_  (int fd, DisconnectStatus error){
			ioEngine_.add_close(fd);

			log_Close_(fd, error);
		}

	private:
		void log_Close_  (int fd, DisconnectStatus error);

	private:
		bool client_Worker_(int fd, Client<MemoryManager> &client);

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

		constexpr static
		void expireFD_(){};

	private:
		template<Logger::Level level = Logger::DEBUG>
		void log_(const char *msg) const{
			logger_fmt<level>(FMT_MASK,   msg, connectedClients());
		}

		template<Logger::Level level = Logger::NOTICE>
		void log_(const char *msg, int const fd) const{
			logger_fmt<level>(FMT_MASK_2, msg, connectedClients(), fd	);
		}

	private:
		IOEngine		ioEngine_;
		Worker			worker_;

		MemoryManager			&mm_;

		bool			keepProcessing_		= true;

		uint32_t		conf_rlimitNoFile_	;
		uint32_t		conf_maxClients_	;

		size_t			conf_bufferCapacity_	;
		size_t			conf_maxRequestSize_	;

		FDStorage		clients_{ conf_rlimitNoFile_, conf_maxClients_ };
	};

} // namespace net

// ===========================

#include "asynccompletionloop.h.cc"

#endif

