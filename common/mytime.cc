#include "mytime.h"

#include <chrono>

#include <time.h>	// localtime, strftime

uint32_t MyTime::now32() noexcept{
	const auto now = std::chrono::system_clock::now().time_since_epoch();

	const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);

	return (uint32_t) sec.count();
}

uint64_t MyTime::now64() noexcept{
	// thanks to Howard Hinnant for this
	const auto now = std::chrono::system_clock::now().time_since_epoch();

	const auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
	const auto mil = std::chrono::duration_cast<std::chrono::microseconds>(now - sec);

	const auto sec_int = (uint32_t) sec.count();
	const auto mil_int = (uint32_t) mil.count();

	return to64(sec_int, mil_int);
}

std::string_view MyTime::toString(uint32_t const date2, const char *format, char *buffer) noexcept{
	time_t const date = date2;

	if (date == 0)
		return "";

	struct tm *tm = localtime(& date);

	if (tm == nullptr)
		return "";

	strftime(buffer, BUFFER_SIZE, format, tm);

	return buffer;
}

