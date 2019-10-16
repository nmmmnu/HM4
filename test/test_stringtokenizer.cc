#include "stringtokenizer.h"

#include "mytest.h"

#include <vector>

#include <iostream>

MyTest mytest;

int main(){
	mytest.begin("StringTokenizer");

	{
		StringTokenizer const st{ "hello world!" };

		std::vector<std::string_view> const v{ std::begin(st), std::end(st) };
		std::vector<std::string_view> const r{ "hello", "world!" };

		mytest("fw", v == r);
	}

	{
		StringTokenizer const st{ " hello  world! " };

		std::vector<std::string_view> const v{ std::begin(st), std::end(st) };
		std::vector<std::string_view> const r{ "", "hello", "", "world!" };

		mytest("fw gaps", v == r);
	}

	// ==============

	{
		StringTokenizer const st{ "hello world!" };

		std::vector<std::string_view> v;

		// ugly but...
//		for(auto it = std::make_reverse_iterator(std::end(st)); it != std::make_reverse_iterator(std::begin(st)); ++it)
		for(auto it = std::end(st); it != std::begin(st); --it)
			v.push_back(*std::prev(it));

		std::vector<std::string_view> const r{ "world!", "hello" };

		mytest("rw", v == r);
	}

	{
		StringTokenizer const st{ " hello  world! " };

		std::vector<std::string_view> v;

		for(auto it = std::end(st); it != std::begin(st); --it)
			v.push_back(*std::prev(it));

		std::vector<std::string_view> const r{ "world!", "", "hello", "" };

		mytest("rw", v == r);
	}

	return mytest.end();
}


