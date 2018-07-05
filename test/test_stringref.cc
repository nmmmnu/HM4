#include "stringref.h"

#include "mytest.h"

MyTest mytest;

#include <sstream>
#include <iostream>
#include <iomanip>

// ==================================

constexpr
const char	hello_c[]	= "hello";
constexpr
const char	bla_c[]		= "bla";

const char	*hello		= "hello";
const char	*bla		= "bla";

// ==================================

static void fn_fastEmptyChar();
static void fn_compare();
static void fn_concatenate();

static void sr_constexpr();

static void sr_empty();
static void sr_char();
static void sr_std_string();

static void sr_test();

// ==================================

int main(){
	fn_fastEmptyChar();
	fn_compare();
	fn_concatenate();

	sr_constexpr();

	sr_empty();
	sr_char();
	sr_std_string();

	sr_test();

	return mytest.end();
}

// ==================================

static void fn_fastEmptyChar(){
	mytest.begin("fastEmptyChar()");

	const char *s = nullptr;

	mytest("empty",		StringRef::fastEmptyChar(s)				);
	mytest("! empty",	! StringRef::fastEmptyChar(hello)			);
	mytest("! empty",	! StringRef::fastEmptyChar(hello, strlen(hello))	);

	mytest("null",		StringRef::fastEmptyChar(s, 5)				);
	mytest("zero",		StringRef::fastEmptyChar(hello, 0)			);
	mytest("null zero",	StringRef::fastEmptyChar(s, 0)				);
}

static void fn_compare(){
	mytest.begin("compare()");

	mytest("null",		! StringRef::compare(	"", 0,			"", 0)			);

	mytest("<",		StringRef::compare(	"a", 1,			"b", 1) < 0		);
	mytest(">",		StringRef::compare(	"b", 1,			"a", 1) > 0		);

	mytest("eq",		! StringRef::compare(	hello, strlen(hello),	hello, strlen(hello))	);

	mytest("! eq",		StringRef::compare(	hello, 0,		hello, strlen(hello))	);
	mytest("! eq",		StringRef::compare(	hello, 1,		hello, strlen(hello))	);
	mytest("! eq",		StringRef::compare(	hello, strlen(hello),	hello, 0)		);
	mytest("! eq",		StringRef::compare(	hello, strlen(hello),	hello, 1)		);


	// lower_bound bug 2017-06-14
	{
		const char *lb_hello = "hello~";

		int const lb_correct  = StringRef::compare("a", 1, "b", 1);

		mytest("lower_bound",	StringRef::compare(
						hello,		strlen(hello),
						lb_hello,	strlen(lb_hello)
					) == + lb_correct
		);

		mytest("upper_bound",	StringRef::compare(
						lb_hello,	strlen(lb_hello),
						hello,		strlen(hello)
					) == - lb_correct
		);

	}

}

static void fn_concatenate(){
	mytest.begin("concatenate()");

	const std::string s = StringRef::concatenate( { "1", "2", "3", "4", "5", "hello" } );

	mytest("concatenate",	s == "12345hello"		);
}

static void sr_constexpr(){
	mytest.begin("StringRef constexpr");

	constexpr StringRef sss{ "some constexpr string..." };
	constexpr StringRef s{ sss };

	constexpr const char *data = s.data();
	mytest("constexpr data",	s == data		);

	constexpr auto size = s.size();
	mytest("constexpr size",	size == s.size()	);

	constexpr auto ch = s[0];
	mytest("constexpr []",	ch == s[0]			);

	constexpr auto it = s.begin();
	mytest("constexpr *it",	*it == s[0]			);
}

static void sr_empty(){
	mytest.begin("StringRef empty");

	constexpr StringRef sr;

	mytest("size",		sr.size() == 0						);
	mytest("data",		strcmp(sr.data(), "") == 0				);
	mytest("empty",		sr.empty()						);
}

static void sr_char(){
	mytest.begin("StringRef char");

	const StringRef sr = { hello, 1 };
	const char ch[] = { hello[0], '\0' };

	mytest("size",		sr.size() == 1				);

	mytest("eq char *",	sr == ch				);
	mytest("eq  string",	sr == std::string(ch)			);
	mytest("eq StringRef",	sr == StringRef(ch)			);
	mytest("eq char",	sr == ch[0]				);
}

static void sr_std_string(){
	mytest.begin("StringRef vs std::string");

	const std::string src = hello;

	const StringRef sr = src;
	const std::string s = sr;

	mytest("size",		s.size() == strlen(hello)		);
	mytest("equal",		s == hello				);

	try{
		const StringRef x{ std::string("hello") };
		mytest("decay",		false				);
	}catch(const std::logic_error &e){
		mytest("decay",		true				);
	}
}

static void sr_test(){
	mytest.begin("general");

	const StringRef &sr = hello;

	mytest("! empty",	! sr.empty()				);

	mytest("size",		sr.size() == strlen(hello)		);
	mytest("size",		sr.length() == strlen(hello)		);

	mytest("data",		strcmp(sr.data(), hello) == 0		);
	mytest("data",		strcmp(sr.c_str(), hello) == 0		);

	mytest("cmp char * MO",	sr.compare("bello") != 0		);
	mytest("cmp char *",	sr.compare(hello) == 0			);
	mytest("cmp char []",	sr.compare(hello_c) == 0		);
	mytest("cmp string",	sr.compare(std::string(hello)) == 0	);
	mytest("cmp StringRef",	sr.compare(StringRef(hello)) == 0	);

	mytest("cmp ___",	sr.compare("_aaaa") > 0			);
	mytest("cmp ~~~",	sr.compare("~aaaa") < 0			);

	mytest("== char *",	sr == hello				);
	mytest("== char []",	sr == hello_c				);
	mytest("== string",	sr == std::string(hello)		);
	mytest("== StringRef",	sr == StringRef(hello)			);

	mytest("eq char *",	sr.equals(hello, strlen(hello))		);

	mytest("|= char *",	sr != bla				);
	mytest("!= char []",	sr != bla_c				);
	mytest("|= string",	sr != std::string(bla)			);
	mytest("|= StringRef",	sr != StringRef(bla)			);

	mytest("[] char *",	sr[0] == hello[0]			);

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

	{
		std::stringstream ss;

		ss << sr;

		mytest("<<",		sr == ss.str()				);
	}

	{
		std::stringstream ss;

		ss << std::setfill('.') << std::left << std::setw(5) << StringRef{"x"};

		mytest("<< L",		ss.str() == "x...."			);
	}

	{
		std::stringstream ss;

		ss << std::setfill('.') << std::right << std::setw(5) << StringRef{"x"};

		mytest("<< R",		ss.str() == "....x"			);
	}

}

