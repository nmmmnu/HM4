#ifndef MY_TIME_H_
#define MY_TIME_H_

#include <cstdint>
#include <array>
#include <string_view>
#include <algorithm>

namespace mytime{
	// 1980-01-01 00:00:00
	// size 19
	constexpr std::size_t to_string_buffer_t_size	= 24;
	using to_string_buffer_t = std::array<char, to_string_buffer_t_size>;

	constexpr std::string_view TIME_FORMAT_STANDARD		= "%Y-%m-%d %H:%M:%S";
	constexpr std::string_view TIME_FORMAT_NUMBER		= "%Y%m%d.%H%M%S";
	constexpr std::string_view DATE_FORMAT_STANDARD2	= "%Y-%m-%d";
	constexpr std::string_view DATE_FORMAT_NUMBER2		= "%Y%m%d";

	uint32_t now32() noexcept;
	uint64_t now64() noexcept;

	inline uint64_t now() noexcept{
		return now64();
	}

	std::array<uint32_t,2> nowMix() noexcept;

	constexpr uint64_t to64(uint32_t const sec, uint32_t const usec = 0) noexcept{
		return uint64_t{sec} << 32 | usec;
	}

	constexpr uint32_t to32(uint64_t const timestamp) noexcept{
		return uint32_t( timestamp >> 32 );
	}

	constexpr uint32_t toUsec(uint64_t const timestamp) noexcept{
		return uint32_t( timestamp & 0xFFFF'FFFF );
	}

	std::string_view toString(uint32_t date, std::string_view const format, to_string_buffer_t &buffer) noexcept;

	inline std::string_view toString(uint64_t date, std::string_view const format, to_string_buffer_t &buffer) noexcept{
		return toString(to32(date), format, buffer);
	}

	inline std::string_view toString(std::string_view const format, to_string_buffer_t &buffer) noexcept{
		return toString(now(), format, buffer);
	}

	inline uint64_t addTime(uint64_t const date, uint32_t const expiration) noexcept{
		uint64_t const a = date;
		uint64_t const b = to64(expiration);

		uint64_t const max = UINT64_MAX; // lets not add <limits>

		if (max - a < b)
			return max; // overflow detected.
		else
			return a + b;
	}

	inline bool expired(uint64_t const date, uint32_t const expiration) noexcept{
		return addTime(date, expiration) < now();
	}



	struct MyTimer{
		bool expired(uint32_t const timeout) const noexcept{
			return time_ + timeout < now_();
		}

		void restart() noexcept{
			time_ = now_();
		}

	private:
		static uint32_t now_() noexcept{
			return mytime::now32();
		}

	private:
		uint32_t time_ = now_();

	};



	struct CrontabControl{
		constexpr CrontabControl(uint32_t timeout) : timeout(timeout){}

		constexpr CrontabControl(uint32_t timeout, uint32_t timeout_min) :
					timeout(timeout == 0 ? 0 : std::max(timeout, timeout_min)){}

		MyTimer		timer;
		uint32_t	timeout;
	};

	template<class F>
	void crontab(CrontabControl &control, F &&f){
		auto &c = control;

		if (c.timeout && c.timer.expired(c.timeout)){
			c.timer.restart();
			f();
		}
	}

} // namespace mytime



using mytime::MyTimer;



#endif

