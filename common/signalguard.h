#ifndef MY_SIGNAL_H_
#define MY_SIGNAL_H_

#include <utility>

enum class Signal : int{
	NONE = 0	,

	INT		,
	TERM		,
	HUP		,
	USR1		,
	USR2
};

class SignalGuard{
public:
	SignalGuard();

	~SignalGuard();

	Signal operator()() const{
		return std::exchange(signal__, Signal::NONE);
	}

public:
	static void update(Signal const signal){
		signal__ = signal;
	}

private:
	static Signal signal__;
};

#endif

