#include "iouringengine.h"
#include "iouringengine.userpack.h"
#include "sockets.h"

#include <cstring>

#include <liburing.h>



static_assert(std::is_same_v<unsigned, uint32_t>);
static_assert(std::is_same_v<int,       int32_t>);



namespace net{
namespace ioengine{

// no need but to show the non class functions...
namespace{

	template<bool AssumeSQEHaveFreeSpace>
	bool add_timeout(struct io_uring *ring, __kernel_timespec *timeout){
		struct io_uring_sqe *sqe = io_uring_get_sqe(ring);

		if constexpr(!AssumeSQEHaveFreeSpace){
			if (!sqe)
				return false;
		}

		sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;

		io_uring_prep_link_timeout(sqe, timeout, 0);

		io_uring_sqe_set_data64(sqe, io_uring_user_pack::DATA_IGNORE);

		return true;
	}

	bool chain_timeout(bool hasTimeout, struct io_uring *ring, __kernel_timespec *timeout, struct io_uring_sqe *sqe){
		if (!hasTimeout)
			return true;

		add_timeout<true>(ring, timeout);

		sqe->flags |= IOSQE_IO_LINK;

		return true;
	}

	constexpr auto convertOp(io_uring_user_pack::Op op){
		using UOp = io_uring_user_pack::Op;

		switch(op){
		default:
		case UOp::CLOSE	 	: return FDOperation::CLOSE	;
		case UOp::ACCEPT	: return FDOperation::ACCEPT	;
		case UOp::READ		: return FDOperation::READ	;
		case UOp::WRITE	 	: return FDOperation::WRITE	;
		}
	}

} // anonymous namespace

struct IOURingEngine::Control{
	struct io_uring		ring;

	__kernel_timespec	timeoutWait;
	__kernel_timespec	timeoutRead;
	__kernel_timespec	timeoutWrite;

	bool			stopped = false;
};



IOURingEngine::IOURingEngine(
			uint32_t /* conf_rlimitNoFile */	,
			uint32_t conf_maxClients		,
			uint32_t conf_maxServersFD		,

			int32_t conf_timeoutWait		,
			int32_t conf_timeoutRead		,
			int32_t conf_timeoutWrite
		) :
				conf_maxClients_	(conf_maxClients		),
				conf_maxServersFD_	(conf_maxServersFD		),
				control_		(std::make_unique<Control>()	){

	events_.reserve(MAX_EVENT_PER_LOOP);

	{
		// init io_uring
		auto const qsize = (conf_maxClients + conf_maxServersFD) * 2 + std::max(RESERVED_FD, conf_maxServersFD);

		struct io_uring_params params;
		memset(&params, 0, sizeof(params));

		if constexpr(1){
			params.flags |= IORING_SETUP_SINGLE_ISSUER;	// 6.0
			params.flags |= IORING_SETUP_COOP_TASKRUN;	// 5.19
			params.flags |= IORING_SETUP_TASKRUN_FLAG;	// 5.19
		}else{
			params.flags |= IORING_SETUP_SINGLE_ISSUER;	// 6.0
			params.flags |= IORING_SETUP_SQPOLL;		// 5.1
		}

		if (auto const ok = io_uring_queue_init_params(qsize, &control_->ring, &params); ok)
			net::perrorExit(10, ok, "io_uring_queue_init");
	}

	{
		auto mkts = [](__kernel_timespec &ts, int32_t sec){
			ts.tv_sec	= sec ? sec : 1;
			ts.tv_nsec	= 0;
		};

		// init timeouts
		mkts(control_->timeoutWait	, conf_timeoutWait	);
		mkts(control_->timeoutRead	, conf_timeoutRead	);
		mkts(control_->timeoutWrite	, conf_timeoutWrite	);
	}
}

IOURingEngine::IOURingEngine(IOURingEngine &&other) noexcept = default;
IOURingEngine &IOURingEngine::operator=(IOURingEngine &&other) noexcept = default;

IOURingEngine::~IOURingEngine(){
	shutdown_<0>();
}

void IOURingEngine::shutdown(){
	shutdown_<1>();
}

template<bool NC>
void IOURingEngine::shutdown_(){
	if (!control_ || control_->stopped)
		return;

	io_uring_queue_exit(&control_->ring);

	if constexpr(NC)
		control_->stopped = true;
}

bool IOURingEngine::add_accept(int fd){
	struct io_uring_sqe *sqe = io_uring_get_sqe(&control_->ring);

	if (!sqe)
		return false;

	io_uring_prep_accept(
		sqe,
		fd,
		nullptr,
		nullptr,
		0
	);

	auto const data64 = io_uring_user_pack::encode(io_uring_user_pack::Op::ACCEPT, fd);

	io_uring_sqe_set_data64(sqe, data64);

	return true;
}

bool IOURingEngine::add_close(int fd) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(&control_->ring);

	if (!sqe)
		return false;

	io_uring_prep_close(
		sqe,
		fd
	);

	auto const data64 = io_uring_user_pack::encode(io_uring_user_pack::Op::CLOSE, fd);

	io_uring_sqe_set_data64(sqe, data64);

	return true;
}

bool IOURingEngine::add_read(int fd, void *buffer, uint32_t size, bool const timeout){
	if (uint32_t const needed = timeout ? 2 : 1; io_uring_sq_space_left(&control_->ring) < needed)
    		return false;

	// guaranteed not nullptr
	struct io_uring_sqe *sqe = io_uring_get_sqe(&control_->ring);

	io_uring_prep_read(
		sqe,
		fd,
		buffer,
		size,
		0
	);

	auto const data64 = io_uring_user_pack::encode(io_uring_user_pack::Op::READ, fd);

	io_uring_sqe_set_data64(sqe, data64);

	return chain_timeout(timeout, &control_->ring, &control_->timeoutRead, sqe);
}

bool IOURingEngine::add_write(int fd, const void *buffer, uint32_t size, bool const timeout){
	if (uint32_t const needed = timeout ? 2 : 1; io_uring_sq_space_left(&control_->ring) < needed)
    		return false;

	// guaranteed not nullptr
	struct io_uring_sqe *sqe = io_uring_get_sqe(&control_->ring);

	io_uring_prep_write(
		sqe,
		fd,
		buffer,
		size,
		0
	);

	auto const data64 = io_uring_user_pack::encode(io_uring_user_pack::Op::WRITE, fd);

	io_uring_sqe_set_data64(sqe, data64);

	return chain_timeout(timeout, &control_->ring, &control_->timeoutWrite, sqe);
}

WaitStatus IOURingEngine::wait(bool const timeout){
	io_uring_submit(&control_->ring);

	struct io_uring_cqe *cqe;

	int const ret = timeout ?
				io_uring_wait_cqe_timeout(&control_->ring, &cqe, &control_->timeoutWait) :
				io_uring_wait_cqe        (&control_->ring, &cqe                        )
	;

	if (ret == -ETIME)
		return WaitStatus::NONE;

	if (ret < 0)
		return WaitStatus::ERROR;

	// loop

	unsigned count = 0;

	events_.clear();

	{
		unsigned head;

		io_uring_for_each_cqe(&control_->ring, head, cqe){
			++count;

			auto const data64 = io_uring_cqe_get_data64(cqe);

			// Ignoring any timeout or cancel on timeouts...
			if (data64 == io_uring_user_pack::DATA_IGNORE)
				continue;

			auto const fd  = io_uring_user_pack::getFD(data64);
			auto const uop = io_uring_user_pack::getOp(data64);
			auto const op  = convertOp(uop);

			// Push accept back, but pass it to events too...
			if constexpr(AUTO_ADD_ACCEPT){
				if (op == FDOperation::ACCEPT)
					add_accept(fd);
			}

			events_.emplace_back(fd, cqe->res, op);

			if (events_.full())
				break;
		}
	}

	// mark seen
	io_uring_cq_advance(&control_->ring, count);

	// we need status as ZERO_RESULTS, but we do not have it yet
	// return events_.empty() ? WaitStatus::ZERO_RESULTS : WaitStatus::OK;
	return WaitStatus::OK;
}

bool IOURingEngine::addServerFD(int fd, size_t const count){
	if (!count)
		return false;

	for(size_t i = 0; i < count; ++i){
		if (!conf_maxServersFD_)
			return false;

		if (!add_accept(fd))
			return false;

		--conf_maxServersFD_;
	}

	return true;
}



} // namespace ioengine
} // namespace

