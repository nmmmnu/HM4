#include "stringhash.h"

#include "mytest.h"

MyTest mytest;

template<class T>
void ss_test(){
	mytest.begin("String Hash");

	using SS = StringHash<T>;

	auto const hello = SS::create("hello");

	auto compare = [hello](const char *s){
		auto a = SS::create(s);

		auto [ b, x ] = SS::compare(hello, a);

		return x;
	};

	mytest("cmp",		compare("hello") == 0			);
	mytest("cmp",		compare("bello") != 0			);

	mytest("cmp ___",	compare("_aaaa") > 0			);
	mytest("cmp ~~~",	compare("~aaaa") < 0			);
}

int main(){
	ss_test<uint32_t>();
	ss_test<uint64_t>();

	return mytest.end();
}

