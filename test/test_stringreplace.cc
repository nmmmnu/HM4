#include "stringreplace.h"

#include "mytest.h"

MyTest mytest;

int main(){
	mytest.begin("MyStringReplace");

	const char *s		= "hello.*.db";
	const char *find	= "*";
	const char *findne	= "@";
	const char *replace	= "20170101";

	const char *result	= "hello.20170101.db";
	const char *resultne	= "hello..db";

	StringReplaceCopy str_replace;

	mytest("", str_replace(s, find		, replace	) == result	);
	mytest("", str_replace(s, find[0]	, replace	) == result	);

	mytest("", str_replace(s, find		, ""		) == resultne	);
	mytest("", str_replace(s, find[0]	, ""		) == resultne	);

	mytest("", str_replace(s, findne	, replace	) == s		);
	mytest("", str_replace(s, findne[0]	, replace	) == s		);

	mytest("", str_replace(s, ""		, replace	) == s		);
	mytest("", str_replace(s, ""		, replace	) == s		);

	return mytest.end();
}

