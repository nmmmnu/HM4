#include "stringtokenizer.h"

#include "mytest.h"

#include <vector>

#include <iostream>

MyTest mytest;

int main(){
	mytest.begin("StringTokenizer");

	{
		StringTokenizer const st{ "hello world!" };

		std::vector<StringRef> const v{ std::begin(st), std::end(st) };
		std::vector<StringRef> const r{ "hello", "world!" };

		mytest("fw", v == r);
	}

	{
		StringTokenizer const st{ " hello  world! " };

		std::vector<StringRef> const v{ std::begin(st), std::end(st) };
		std::vector<StringRef> const r{ "", "hello", "", "world!" };

		mytest("fw gaps", v == r);
	}

	// ==============

	{
		StringTokenizer const st{ "hello world!" };

		std::vector<StringRef> v;

		for(auto it = std::end(st); it != std::begin(st); --it)
			v.push_back(*std::prev(it));

		std::vector<StringRef> const r{ "world!", "hello" };

		mytest("rw", v == r);
	}

	{
		StringTokenizer const st{ " hello  world! " };

		std::vector<StringRef> v;

		for(auto it = std::end(st); it != std::begin(st); --it)
			v.push_back(*std::prev(it));

		std::vector<StringRef> const r{ "world!", "", "hello", "" };

		mytest("rw", v == r);
	}

	return mytest.end();
}


