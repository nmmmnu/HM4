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

	mytest("", StringReplace::replaceByCopy(s, find	,	 replace	) == result	);
	mytest("", StringReplace::replaceByCopy(s, find[0]	, replace	) == result	);

	mytest("", StringReplace::replaceByCopy(s, find		, ""		) == resultne	);
	mytest("", StringReplace::replaceByCopy(s, find[0]	, ""		) == resultne	);

	mytest("", StringReplace::replaceByCopy(s, findne	, replace	) == s		);
	mytest("", StringReplace::replaceByCopy(s, findne[0]	, replace	) == s		);

	mytest("", StringReplace::replaceByCopy(s, ""		, replace	) == s		);
	mytest("", StringReplace::replaceByCopy(s, ""		, replace	) == s		);

	return mytest.end();
}

