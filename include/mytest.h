#ifndef _MY_TEST_H
#define _MY_TEST_H

#include <cstdio>
#include <cstdlib>	// exit

class MyTest{
public:
	constexpr static bool DEFAULT_SHOW = true;
	constexpr static bool DEFAULT_STOP = true;

public:
	MyTest(bool const show = DEFAULT_SHOW, bool const stop = DEFAULT_STOP ) : show(show), stop(stop){}

public:
	void operator()(const char *test, bool const result){
		if (show || result == false)
			printf(" - Testing %-25s %s\n", test, result ? "OK" : "Fail");

		if (stop && result == false)
			exit(1);
	}

public:
	void begin(const char *title){
		printf("\nTesting %s...\n", title);
	}

	int end(){
		printf("\nAll tests passed. You are awesome!!!\n");

		return 0;
	}

private:
	bool show;
	bool stop;
};

#endif

