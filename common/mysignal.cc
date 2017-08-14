#include "mysignal.h"

#include <signal.h>

bool mySignalOK = true;

extern "C"{
	static void intHandler(int const sig) {
		signal(sig, SIG_IGN);

		mySignalOK = false;

		signal(sig, intHandler);
	}
}

void prepareSignals(){
	signal(SIGINT,  intHandler);	// Ctrl C
	signal(SIGTERM, intHandler);	// kill -TERM / Shutdown
	signal(SIGHUP,  SIG_IGN);	// kill -HUP
}

void restoreSignals(){
	signal(SIGINT,  SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGHUP,  SIG_DFL);
}

