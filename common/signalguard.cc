#include "signalguard.h"

#include <utility>
#include <iterator>

#include <signal.h>



constexpr int ZERO = static_cast<int>(Signal::NONE);

int g_signal__ = ZERO;



namespace {
	using Handler = SignalGuard::Handler;

	void set_signal(int const signal, Handler func){
		struct sigaction action;

		action.sa_handler = func;
		sigemptyset (&action.sa_mask);
		action.sa_flags = 0;

		sigaction (signal, &action, nullptr);
	}

	auto get_signal(int const signal){
		struct sigaction action;
		sigaction(signal, nullptr, &action);

		return action.sa_handler;
	}

	auto getset_signal(int const signal, Handler func){
		auto x = get_signal(signal);
		set_signal(signal, func);
		return x;
	}
}



namespace {
	constexpr int signals[5] = {
		SIGINT	,	// Ctrl C
		SIGTERM	,	// kill -TERM / Shutdown
		SIGHUP	,
		SIGUSR1	,
		SIGUSR2
	};

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

	void handler(int const signal){
		g_signal__ = signal;
	}

} // anonymous namespace



SignalGuard::SignalGuard(){
	g_signal__ = ZERO;

	for(size_t i = 0; i < std::size(signals); ++i)
		old[i] = getset_signal(signals[i], handler);
}

SignalGuard::~SignalGuard(){
	for(size_t i = 0; i < std::size(signals); ++i)
		set_signal(signals[i], old[i]);
}

Signal SignalGuard::operator()() const{
	return translate(
		std::exchange(g_signal__, ZERO)
	);
}

const char *SignalGuard::toString(Signal const signal) const{
	constexpr const char *str[] = {
		"NONE"	,

		"INT"	,
		"TERM"	,
		"HUP"	,
		"USR1"	,
		"USR2"
	};

	return str[static_cast<int>(signal)];
}

