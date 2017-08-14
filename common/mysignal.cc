#include "mysignal.h"

#include <signal.h>

bool mySignalOK = true;

// ----------------------------------

constexpr int SIGNALS_CATCH[] = {
	SIGINT,		// Ctrl C
	SIGTERM		// kill -TERM / Shutdown
};

constexpr int SIGNALS_IGNORE[] = {
	SIGHUP,		// kill -HUP
	SIGUSR1,
	SIGUSR2
};

// ----------------------------------

extern "C"{
	static void intHandler(int const sig) {
		signal(sig, SIG_IGN);

		mySignalOK = false;

		signal(sig, intHandler);
	}
}

// ----------------------------------

template<class CONTAINER, class FUNC>
void signal_(const CONTAINER &signals, FUNC *func){
	for(auto const sig : signals)
		signal(sig,  func);
}

// ----------------------------------

void mySignalPrepare(){
	signal_(SIGNALS_CATCH,  intHandler);
	signal_(SIGNALS_IGNORE, SIG_IGN);
}

void mySignalRestore(){
	signal_(SIGNALS_CATCH,  SIG_DFL);
	signal_(SIGNALS_IGNORE, SIG_DFL);
}

