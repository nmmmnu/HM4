#ifndef _MY_TEST_H
#define _MY_TEST_H

#define FMT_HEADER_ONLY
#include "fmt/core.h"

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
			fmt::print(" - Testing {:25} {}\n", test, result ? "OK" : "Fail");

		if (stop && result == false)
			exit(1);
	}

public:
	void begin(const char *title){
		fmt::print("\nTesting {}...\n", title);
	}

	int end(){
		fmt::print("\nAll tests passed. You are awesome!!!\n");

		return 0;
	}

private:
	bool show;
	bool stop;
};

#endif

