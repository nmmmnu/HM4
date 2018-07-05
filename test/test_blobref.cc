#include "blobref.h"

#include <cstdint>
#include <cstring>

#include <iostream>
#include <iomanip>

#include "myendian.h"

#include "mytest.h"

MyTest mytest;

void test_blobref(){
	mytest.begin("BlobRef");

	constexpr size_t SIZE = 256;
	char mem[SIZE];

	for(size_t i = 0; i < SIZE; ++i)
		mem[i] = (char) i;

	BlobRef br{ mem };

	mytest("as()",		*br.as<uint16_t>(0x00) == htobe<uint16_t>(0x0001)	);
	mytest("as()",		*br.as<uint16_t>(0x0E) == htobe<uint16_t>(0x0E0F)	);
	mytest("as()",		*br.as<uint32_t>(0x10) == htobe<uint32_t>(0x10111213)	);

	{
	const char *s = br.as<char>('a', 5);
	mytest("as() str",		strncmp(s, "abcde", 5) == 0);

	// relative
	mytest("as() rel",	*br.as<char>(s + 5) == 'f');
	}

	{
	struct TestStruct{
		uint16_t	i;
		char		c;
		char		s[4];
	}__attribute__((__packed__));

	const TestStruct *st = br.as<TestStruct>(0x50);
	mytest("struct",	st->i    == htobe<uint16_t>(0x5051)	);
	mytest("struct",	st->c    == 0x52			);
	mytest("struct",	st->s[0] == 0x53			);
	mytest("struct",	st->s[1] == 0x54			);
	mytest("struct",	st->s[2] == 0x55			);
	mytest("struct",	st->s[3] == 0x56			);

	// relative
	const char *stc = (const char *) st;
	mytest("struct",	*br.as<char>(stc + sizeof(TestStruct)) == 0x57);
	}

	{
	size_t const max = SIZE / sizeof(uint64_t);

	const uint64_t *u64 = br.as<uint64_t>(0, max);
	mytest("end",		u64 != nullptr	);
	mytest("end",		u64[      0] == htobe<uint64_t>(0x0001020304050607)	);
	mytest("end",		u64[max - 1] == htobe<uint64_t>(0xf8f9fafbFCFDFEFF)	);

	// relative
	const uint64_t *p = & u64[max - 2];
	mytest("end",		*br.as<uint64_t>(p + 1) == u64[max - 1]	);
	}

	{
	size_t const max = SIZE / sizeof(uint64_t) ;

	// after last
	mytest("past end",	br.as<uint64_t>(0, max + 1 ) == nullptr		);
	}

	{
	size_t const max = SIZE / sizeof(uint64_t) ;

	const uint64_t *u64 = br.as<uint64_t>(0, max);

	// relative
	// after last
	const uint64_t *p = & u64[max - 1];
	mytest("after last",	br.as<uint64_t>(p + 1) == nullptr	);
	}

	// zero size
	mytest("zero size",	br.as<char>(0x00, 0) == nullptr	);

	// nullptr
	mytest("nullptr",	br.as<char>(nullptr) == nullptr	);
}

int main(){
	test_blobref();

	return mytest.end();
}

