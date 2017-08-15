#include "signalguard.h"

#include <signal.h>

bool SignalGuard::ok__ = false;

constexpr int SIGNALS_CATCH[] = {
	SIGINT,		// Ctrl C
	SIGTERM		// kill -TERM / Shutdown
};

constexpr int SIGNALS_IGNORE[] = {
	SIGHUP,		// kill -HUP
	SIGUSR1,
	SIGUSR2
};

template<class CONTAINER, class FUNC>
void signal_(const CONTAINER &signals, FUNC *func){
	for(auto const sig : signals)
		signal(sig,  func);
}

// ----------------------------------

void SignalGuard::setHandler__(){
	signal_(SIGNALS_CATCH,  handler__);
	signal_(SIGNALS_IGNORE, SIG_IGN);
}

void SignalGuard::restoreHandler__(){
	signal_(SIGNALS_CATCH,  SIG_DFL);
	signal_(SIGNALS_IGNORE, SIG_DFL);
}

