#include "signalguard.h"

#include <signal.h>

#include <cstdio>

namespace {
	constexpr int signals[] = {
		SIGINT,		// Ctrl C
		SIGTERM,	// kill -TERM / Shutdown
		SIGHUP,
		SIGUSR1,
		SIGUSR2
	};

	template<class CONTAINER, class FUNC>
	void setup_signal(const CONTAINER &signals, FUNC *func){
		for(auto const sig : signals)
			signal(sig,  func);
	}

	auto translate(int const signal){
		switch(signal){
		default		: return Signal::NONE	;

		case SIGINT	: return Signal::INT	;
		case SIGTERM	: return Signal::TERM	;
		case SIGHUP	: return Signal::HUP	;
		case SIGUSR1	: return Signal::USR1	;
		case SIGUSR2	: return Signal::USR2	;
		}
	}

	const char *toString(int const signal){
		switch(signal){
		default		: return "NONE"	;

		case SIGINT	: return "INT"	;
		case SIGTERM	: return "TERM"	;
		case SIGHUP	: return "HUP"	;
		case SIGUSR1	: return "USR1"	;
		case SIGUSR2	: return "USR2"	;
		}
	}

	void handler(int const signal){
		(void) toString;

		auto const t = translate(signal);
		SignalGuard::update(t);
	}

} // anonymous namespace

Signal SignalGuard::signal__;

SignalGuard::SignalGuard(){
	update(Signal::NONE);
	setup_signal(signals,  handler);
}

SignalGuard::~SignalGuard(){
	setup_signal(signals,  SIG_DFL);
}



