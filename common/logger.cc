#include "logger.h"

Logger g_log;

Logger &getLoggerSingleton(){
	return g_log;
}

