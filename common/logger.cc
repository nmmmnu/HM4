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

}

my_logger::Logger &getLoggerSingleton(){
	// avoid initialization order fiasco
	static my_logger::Logger g_log;

	return g_log;
}


