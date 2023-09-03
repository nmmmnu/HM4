#include "logger.h"
#include "mytime.h"

namespace my_logger{

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

	Logger g_log;
}

my_logger::Logger &getLoggerSingleton(){
	return my_logger::g_log;
}


