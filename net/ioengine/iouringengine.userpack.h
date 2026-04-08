#ifndef IO_URING_SELECTOR_USER_PASK_H_
#define IO_URING_SELECTOR_USER_PASK_H_

#include <limits>

namespace net::ioengine::io_uring_user_pack{

	constexpr uint64_t DATA_IGNORE = std::numeric_limits<uint64_t>::max();
	constexpr uint64_t U32_        = std::numeric_limits<uint32_t>::max();

	enum class Op : uint64_t {
		READ   = 0ULL << 32,
		WRITE  = 1ULL << 32,
		ACCEPT = 2ULL << 32,
		CLOSE  = 3ULL << 32
	};

	constexpr uint64_t OP_MASK = U32_ << 32; // 32 bits
	constexpr uint64_t FD_MASK = U32_ <<  0; // 32 bits

	constexpr uint64_t encode(Op op, int32_t fd){
		return
			static_cast<uint64_t>(op) |
			static_cast<uint64_t>(fd)
		;
	}

	constexpr auto getOp(uint64_t data){
		return static_cast<Op>(data & OP_MASK);
	}

	constexpr auto getFD(uint64_t data){
		return static_cast<uint32_t>(data);
	}

} // namespace net::ioengine::io_uring_user_pack

#endif

