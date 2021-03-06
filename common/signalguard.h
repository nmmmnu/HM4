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
	using Handler = void(int);

	SignalGuard();

	~SignalGuard();

	Signal operator()() const;

	static const char *toString(Signal const signal);

private:
	Handler *old[5];
};

#endif

