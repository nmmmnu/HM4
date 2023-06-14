#include "logger.h"
#include "mytime.h"

Logger g_log;

Logger &getLoggerSingleton(){
	return g_log;
}

void Logger::LoggerStream::outputTime_(){
	using namespace mytime;

	to_string_buffer_t buffer;

	os_ << toString(now(), TIME_FORMAT_STANDARD, buffer);
}

