#include "stringtokenizer.h"

#include "mytest.h"

MyTest mytest;

int main(){
	mytest.begin("StringTokenizer");

	{
		StringTokenizer const st{ "hello  world" };

		std::vector<StringRef> const v{ std::begin(st), std::end(st) };

		mytest("", v.size() == 2);
		mytest("", v[0] == "hello");
		mytest("", v[1] == "world");
	}

	{
		StringTokenizer const st{ " hello world " };

		std::vector<StringRef> const v{ std::begin(st), std::end(st) };

		mytest("", v.size() == 3);
		mytest("", v[0] == "");
		mytest("", v[1] == "hello");
		mytest("", v[2] == "world");
	}

	return mytest.end();
}

