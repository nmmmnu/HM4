#ifndef IO_IOURING_ENGINE_H_
#define IO_IOURING_ENGINE_H_

#include "staticvector.h"

#include "ioenginedefs.h"

#include <memory>

#include <cstdint>

namespace net{
namespace ioengine{



class IOURingEngine{
	constexpr static bool		AUTO_ADD_ACCEPT		= true;

	constexpr static uint32_t	RESERVED_FD		= 32;

	constexpr static int32_t	DEFAULT_TIMEOUT_WAIT	= 5;
	constexpr static int32_t	DEFAULT_TIMEOUT_READ	= 1 * 60;
	constexpr static int32_t	DEFAULT_TIMEOUT_WRITE	= 1 * 60;

	constexpr static uint32_t	MAX_EVENT_PER_LOOP	= 256; // 6 KB

public:
	static inline constexpr std::string_view NAME = "io_uring";

	IOURingEngine(	uint32_t conf_maxClients	,
			uint32_t conf_maxServerFD	,

			int32_t conf_timeoutWait	= DEFAULT_TIMEOUT_WAIT	,
			int32_t conf_timeoutRead	= DEFAULT_TIMEOUT_READ	,
			int32_t conf_timeoutWrite	= DEFAULT_TIMEOUT_WRITE
		);

	IOURingEngine(IOURingEngine &&other) noexcept;
	IOURingEngine &operator=(IOURingEngine &&other) noexcept;

	~IOURingEngine();

	void shutdown();

private:
	bool add_accept	(int fd);

	template<bool NC>
	void shutdown_();

public:
	bool add_close	(int fd);
	bool add_read	(int fd,       void *buffer, uint32_t size, bool timeout);
	bool add_write	(int fd, const void *buffer, uint32_t size, bool timeout);

	WaitStatus wait(bool timeout);

public:
	auto begin() const{
		return events_.begin();
	}

	auto end() const{
		return events_.end();
	}

public:
	bool addServerFD(int fd, size_t count = 1);

private:
	struct Control;

	using EventVector = StaticVector<FDEvent, MAX_EVENT_PER_LOOP>;

private:
	uint32_t	conf_maxClients_	;
	uint32_t	conf_rlimitNoFile_	;

	EventVector	events_			;

	std::unique_ptr<Control>
			control_		;
};



} // namespace ioengine
} // namespace

#endif

