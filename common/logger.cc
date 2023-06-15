#include "logger.h"
#include "mytime.h"

Logger g_log;

Logger &getLoggerSingleton(){
	return g_log;
}

void Logger::LoggerStream::outputBanner_(const char *banner){
	if (writting_){
		using namespace mytime;

		to_string_buffer_t buffer;

		os_
			<< toString(now(), TIME_FORMAT_STANDARD, buffer)
			<< ' ' << banner << ' '
		;
	}
}

