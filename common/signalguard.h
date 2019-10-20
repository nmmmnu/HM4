#ifndef MY_SIGNAL_H_
#define MY_SIGNAL_H_

enum class Signal : int{
	NONE	=  0,

	HUP,
	INT,
	TERM,
	USR1,
	USR2
};

class SignalGuard{
public:
	SignalGuard();

	~SignalGuard();

	Signal operator()() const;

private:
	// array of pointers to functions
	void (*old[5])(int);
};

#endif

