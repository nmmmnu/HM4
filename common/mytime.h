#ifndef _MYTIME_H
#define _MYTIME_H

#include <cstdint>
#include <cstring>	// strlen
#include <string_view>

class MyTime{
public:
	// __builtin_strlen is constexpr in clang
	constexpr static size_t BUFFER_SIZE			= __builtin_strlen("1980-01-01 00:00:00") + 1;

	constexpr static const char *TIME_FORMAT_STANDARD	= "%Y-%m-%d %H:%M:%S";
	constexpr static const char *TIME_FORMAT_NUMBER		= "%Y%m%d.%H%M%S";

	constexpr static const char *DATE_FORMAT_STANDARD2	= "%Y-%m-%d";
	constexpr static const char *DATE_FORMAT_NUMBER2	= "%Y%m%d";

	constexpr static const char *DATE_FORMAT_DEFAULT	= TIME_FORMAT_STANDARD;

public:
	static std::string_view toString(char *buffer, uint32_t date, const char *format = DATE_FORMAT_DEFAULT) noexcept;

	static std::string_view toString(char *buffer, uint64_t date, const char *format = DATE_FORMAT_DEFAULT) noexcept{
		return toString(buffer, to32(date), format);
	}

	static std::string_view toString(char *buffer, const char *format = DATE_FORMAT_DEFAULT) noexcept{
		return toString(buffer, now(), format);
	}

	static uint64_t now() noexcept{
		return now64();
	}

	static uint32_t now32() noexcept;
	static uint64_t now64() noexcept;

	static uint64_t addTime(uint64_t const date, uint32_t const expiration) noexcept{
		return date + to64(expiration);
	}

	static bool expired(uint64_t const date, uint32_t const expiration) noexcept{
		return addTime(date, expiration) < now();
	}

	constexpr static uint64_t to64(uint32_t const sec, uint32_t const usec = 0) noexcept{
		return (uint64_t) sec << 32 | usec;
	}

	constexpr static uint32_t to32(uint64_t const timestamp) noexcept{
		return uint32_t( timestamp >> 32 );
	}

	constexpr static uint32_t toUsec(uint64_t const timestamp) noexcept{
		return uint32_t( timestamp & 0xFF'FF'FF'FF );
	}
};


// ===========================


class MyTimer{
public:
	bool expired(uint32_t const timeout) const noexcept{
		return time_ + timeout < now_();
	}

	void restart() noexcept{
		time_ = now_();
	}

private:
	static uint32_t now_() noexcept{
		return MyTime::now32();
	}

private:
	uint32_t time_ = now_();

};

#endif

