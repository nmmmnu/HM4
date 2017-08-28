#ifndef MY_SIGNAL_H_
#define MY_SIGNAL_H_

class SignalGuard{
public:
	SignalGuard(){
		ok__ = true;
		setHandler__();
	}

	~SignalGuard(){
		restoreHandler__();
		ok__ = false;
	}

	operator bool(){
		return ok__;
	}

private:
	static void setHandler__();
	static void restoreHandler__();

private:
	static void handler__(int const /* sig */){
		ok__ = false;
	}

private:
	static bool ok__;
};

#endif

