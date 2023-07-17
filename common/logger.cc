#include "logger.h"
#include "mytime.h"



namespace my_logger{
	Logger g_log;

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
	return my_logger::g_log;
}


#if 0
		[[nodiscard]]
		auto startup() const{
			return get(Level::STARTUP	);
		}

		[[nodiscard]]
		auto fatal() const{
			return get(Level::FATAL		);
		}

		[[nodiscard]]
		auto error() const{
			return get(Level::ERROR		);
		}

		[[nodiscard]]
		auto warning() const{
			return get(Level::WARNING	);
		}

		[[nodiscard]]
		auto notice() const{
			return get(Level::NOTICE	);
		}

		[[nodiscard]]
		auto debug() const{
			return get(Level::DEBUG		);
		}
#endif

