#ifndef MY_LOGGER_H_
#define MY_LOGGER_H_

#include <iostream>
#define FMT_HEADER_ONLY
#include "fmt/core.h"
#include "fmt/ostream.h"

struct Logger{
	enum Level{
		STARTUP	= 0,
		FATAL	= 1,
		ERROR	= 2,
		WARNING	= 3,
		NOTICE	= 4,
		DEBUG	= 5
	};

	constexpr static Level DEFAULT_LEVEL = NOTICE;

	Logger(Level level = DEBUG) : level_(level){}

private:
	/*
		Black	\033[30m	\033[40m
		Red	\033[31m	\033[41m
		Green	\033[32m	\033[42m
		Yellow	\033[33m	\033[43m
		Blue	\033[34m	\033[44m
		Purple	\033[35m	\033[45m
		Cyan	\033[36m	\033[46m
		White	\033[37m	\033[47m

		Bold	\033[1m
		Reset	\033[0m
	*/

	constexpr static inline const char *banner[]{
		"\033[1;37;42m"	"[STARTUP]"	"\033[0m",
		"\033[1;37;41m"	"[ FATAL ]"	"\033[0m",
		"\033[1;31m"	"[ ERROR ]"	"\033[0m",
		"\033[1;33m"	"[WARNING]"	"\033[0m",
		"\033[32m"	"[NOTICES]"	"\033[0m",
		"\033[36m"	"[ DEBUG ]"	"\033[0m"
	};

public:
	[[nodiscard]]
	auto get(Level level) const{
		bool const writting = level <= level_;

		return LoggerStream(banner[level], writting);
	}

	template<Level level>
	[[nodiscard]]
	auto get() const{
		return get(level	);
	}

	[[nodiscard]]
	auto startup() const{
		return get(STARTUP	);
	}

	[[nodiscard]]
	auto fatal() const{
		return get(FATAL	);
	}

	[[nodiscard]]
	auto error() const{
		return get(ERROR	);
	}

	[[nodiscard]]
	auto warning() const{
		return get(WARNING	);
	}

	[[nodiscard]]
	auto notice() const{
		return get(NOTICE	);
	}

	[[nodiscard]]
	auto debug() const{
		return get(DEBUG	);
	}

	[[nodiscard]]
	auto getLevel() const{
		return level_;
	}

	void setLevel(Level level){
		level_ = level;
	}

	void setLevel(unsigned level){
		level_ = [level](){
			switch(level){
			case 0:  return STARTUP	;
			case 1:  return FATAL	;
			case 2:  return ERROR	;
			case 3:  return WARNING	;
			case 4:  return NOTICE	;
			case 5:  return DEBUG	;
			};

			return DEFAULT_LEVEL;
		}();
	}

private:
	struct LoggerStream{
		LoggerStream(const char *banner, bool writting, std::ostream &os = std::clog) :
					writting_	(writting	),
					os_		(os		){

			outputBanner_(banner);
		}

		~LoggerStream(){
			if (writting_)
				os_ << '\n';
		}

		template<typename T>
		auto &operator <<(T const &a) const{
			if (writting_)
				os_ << a << ' ';

			return *this;
		}

		template<typename ...Args>
		void fmt(Args &&...args){
			if (writting_)
				fmt::print(os_, std::forward<Args>(args)...);
		}

	private:
		void outputBanner_(const char *banner);

	private:
		bool		writting_;
		std::ostream	&os_;
	};

private:
	Level	level_;
};



[[nodiscard]]
Logger &getLoggerSingleton();

template<Logger::Level level>
[[nodiscard]]
auto logger(){
	auto const &x = getLoggerSingleton();

	return x.get(level);
}

template<Logger::Level level, typename ...Args>
void logger_fmt(Args &&...args){
	return logger<level>().fmt(std::forward<Args>(args)...);
}

#endif
