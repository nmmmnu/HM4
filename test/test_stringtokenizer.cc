#include "stringtokenizer.h"

#include "mytest.h"

MyTest mytest;

int main(){
	mytest.begin("StringTokenizer");

	const char *s1 = "hello world";

	StringTokenizer st1{ s1 };

	mytest("", st1.getNext() == "hello");
	mytest("", st1.getNext() == "world");
	mytest("", st1.getNext().empty());
	mytest("", st1.getNext().empty());
	mytest("", st1.getNext().empty());

	const auto v1 = st1.getAll();

	mytest("", v1.size() == 2);
	mytest("", v1[0] == "hello");
	mytest("", v1[1] == "world");

	const char *s2 = " hello world ";

	StringTokenizer st2{ s2 };

	mytest("", st2.getNext().empty());
	mytest("", st2.getNext() == "hello");
	mytest("", st2.getNext() == "world");
	mytest("", st2.getNext().empty());
	mytest("", st2.getNext().empty());
	mytest("", st2.getNext().empty());

	const auto v2 = st2.getAll();

	mytest("", v2.size() == 3);
	mytest("", v2[0] == "");
	mytest("", v2[1] == "hello");
	mytest("", v2[2] == "world");

	return mytest.end();
}

