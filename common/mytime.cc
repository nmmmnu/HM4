#include "mytime.h"

#include <chrono>

#include <time.h>	// localtime, strftime

namespace mytime{

	uint32_t now32() noexcept{
		const auto now = std::chrono::system_clock::now().time_since_epoch();

		const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);

		return (uint32_t) sec.count();
	}

	uint64_t now64() noexcept{
		auto const [sec_int, mil_int] = nowMix();

		return to64(sec_int, mil_int);
	}

	std::array<uint32_t,2> nowMix() noexcept{
		// thanks to Howard Hinnant for this
		const auto now = std::chrono::system_clock::now().time_since_epoch();

		const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
		const auto mil = std::chrono::duration_cast<std::chrono::microseconds>(now - sec);

		const auto sec_int = (uint32_t) sec.count();
		const auto mil_int = (uint32_t) mil.count();

		// what is going on here:
		//
		// sec: uint64_t with at least 55 bits is casted to uint32_t.
		// It will be good until year 2106-02-07
		//
		// mil: uint65_t with exact    20 bits is casted to uint32_t.
		// This means there are 12 unused bits.
		//
		// So finally we have uint64_t
		// [32 bit seconds][12 bit unused][20 bits microseconds]

		return std::array<uint32_t,2>{
			sec_int,
			mil_int
		};
	}

	std::string_view toString(uint32_t const date2, std::string_view const format, to_string_buffer_t &buffer) noexcept{
		time_t const date = date2;

		if (date == 0)
			return "";

		struct tm *tm = localtime(& date);

		if (tm == nullptr)
			return "";

		strftime(buffer.data(), buffer.size(), format.data(), tm);

		return buffer.data();
	}

} // namespace MyTime


