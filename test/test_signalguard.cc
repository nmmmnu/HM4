#include "signalguard.h"

#include <signal.h>

#include <cstdio>

Signal s1 = Signal::NONE;
Signal s2 = Signal::NONE;

namespace{

	void raise_signal(int const signal, SignalGuard const &sg, Signal &s){
		raise(signal);
		s = sg();
	}

	bool check(Signal const s, Signal const expect){
		bool result = s == expect;
		printf("%s %s\n", SignalGuard::toString(s1), result ? "OK" : "FAIL") ;
		return result;
	}

} // namespace

int main(){
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);

	{
		SignalGuard sg1;

		{
			SignalGuard sg2;
			raise_signal(SIGUSR2, sg2, s2);
		}

		raise_signal(SIGUSR1, sg1, s1);
	}

	// these should be ignored...
	raise(SIGUSR1);
	raise(SIGUSR2);

	bool b =
		check(s1, Signal::USR1)	&&
		check(s2, Signal::USR2)
	;

	if (b){
		printf("Test passed, you are awesome!\n");
	}
}

