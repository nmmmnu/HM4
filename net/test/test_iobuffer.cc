#include "iobuffer.h"

#include "mytest.h"


MyTest mytest;


using Buffer = net::IOBuffer;


inline int cmp(const std::string &s, const char *b){
	return s.compare(/* pos */ 0, s.size(), b);
}


void testPush(const char *data, Buffer &b, std::string &s, bool const expect = true){
	bool const result = b.push(data);

	if (expect)
		s += data;

	mytest("push",	result == expect	);
	mytest("value",	cmp(s, b.data()) == 0	);
}


void testPop(size_t const size, Buffer &b, std::string &s, bool const expect = true){
	bool const result = b.pop(size);

	if (expect)
		s.erase(0, size);

	mytest("pop", result == expect);

	if (s.empty()){
		mytest("size zero",	b.size() == 0		);
	}else{
		mytest("value",		cmp(s, b.data()) == 0	);
	}
}


int test(){
	mytest.begin("iobuffer");

	Buffer b;

	std::string s;

	testPush("aaa",	b, s);
	testPush("",	b, s, false);
	testPush("bbb",	b, s);
	testPush("ccc",	b, s);

	testPop( 3,	b, s);
	testPop( 0,	b, s, false);
	testPop( 3,	b, s);
	testPop( 3,	b, s);

	return mytest.end();
}

int main(){
	return test();
}

