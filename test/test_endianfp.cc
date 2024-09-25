#include "myendian_fp.h"
#include "mytest.h"

MyTest mytest;

template<typename T>
void test(const char *title){
	mytest.begin(title);

	mytest("betoh 0",	betoh(T{ 0.0 }  	) == 0	);

	mytest("htobe 0",	betoh(T{ 0.0 }  	) == 0	);

	mytest("htobe/betoh",	betoh(htobe(T{ 5.25 }  )) == 5.25	);
}

int main(){
	test<float >("Endian FP float");
	test<double>("Endian FP double");

	return mytest.end();
}

