#include "smallstring.h"

#include "mytest.h"

MyTest mytest;

#include <sstream>

// ==================================

const char	*hello		= "hello sir how are you?";
const char	*bla		= "bla bla bla...";

// ==================================

static void ss_empty();

template<size_t W>
void ss_test();

// ==================================

int main(){
	ss_empty();
	ss_test<1>();
	ss_test<2>();

	return mytest.end();
}

// ==================================

static void ss_empty(){
	mytest.begin("SmallString empty");

	SmallString<1> sr;

	mytest("size",		sr.size() == 0				);
	mytest("data",		strcmp(sr.data(), "") == 0		);
	mytest("empty",		sr.empty()				);

	for(size_t i = 0; i < sr.capacity(); ++i){
		if ( sr[i] )
			mytest("zerofill",	false	);
	}

	SmallString<1> srz = "X";

	for(size_t i = 1; i < srz.capacity(); ++i){
		if ( srz[i] )
			mytest("zerofill",	false	);
	}
}

template<size_t BYTES>
void ss_test(){
	mytest.begin("general");

	const SmallString<BYTES> &sr = hello;

	auto const cap = sr.capacity();

	mytest("! empty",	! sr.empty()				);

	mytest("size",		sr.size()   == strnlen(hello, cap)	);
	mytest("size",		sr.length() == strnlen(hello, cap)	);

	mytest("data",		strncmp(sr.data(),  hello, cap) == 0	);
	mytest("data",		strncmp(sr.c_str(), hello, cap) == 0	);

	mytest("cmp char * MO",	sr.compare("bello") != 0		);
	mytest("cmp char *",	sr.compare(hello) == 0			);

	mytest("cmp ___",	sr.compare("_aaaa") > 0			);
	mytest("cmp ~~~",	sr.compare("~aaaa") < 0			);

	mytest("eq char *",	sr.equals(hello, strlen(hello))		);

	size_t i = 0;
	for(auto it = sr.begin(); it < sr.end(); ++it){
		mytest("iterator",	sr[i] == *it			);
		++i;
	}

	i = 0;
	for(char const &c : sr){
		mytest("c++11 loop",	sr[i] == c			);
		++i;
	}

	mytest.begin("<iostream> integration");

	std::stringstream ss;

	ss <<  sr;

	const auto sss = ss.str();

	mytest("<<",		sr.equals(sss.data(), sss.size())	);
}

