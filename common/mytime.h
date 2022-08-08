#ifndef _MYTIME_H
#define _MYTIME_H

#include <cstdint>
#include <cstring>	// strlen
#include <array>
#include <string_view>

class MyTime{
public:
	// 1980-01-01 00:00:00
	// size 19
	constexpr static std::size_t to_string_buffer_t_size	= 24;
	using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

	constexpr static std::string_view TIME_FORMAT_STANDARD	= "%Y-%m-%d %H:%M:%S";
	constexpr static std::string_view TIME_FORMAT_NUMBER	= "%Y%m%d.%H%M%S";
	constexpr static std::string_view DATE_FORMAT_STANDARD2	= "%Y-%m-%d";
	constexpr static std::string_view DATE_FORMAT_NUMBER2	= "%Y%m%d";

public:
	static std::string_view toString(uint32_t date, std::string_view const format, to_string_buffer_t &buffer) noexcept;

	static std::string_view toString(uint64_t date, std::string_view const format, to_string_buffer_t &buffer) noexcept{
		return toString(to32(date), format, buffer);
	}

	static std::string_view toString(std::string_view const format, to_string_buffer_t &buffer) noexcept{
		return toString(now(), format, buffer);
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

