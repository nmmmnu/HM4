#ifndef MY_SIGNAL_H_
#define MY_SIGNAL_H_

#include <utility>

// man signal for x86 / ARM

enum class Signal : int{
	NONE	=  0,

	HUP	=  1,
	INT	=  2,
	TERM	= 15,
	USR1	= 10,
	USR2	= 12
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

