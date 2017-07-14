#include "pair.h"
#include "pairblob.h"

using Pair	= hm4::Pair;
using PairBlob	= hm4::PairBlob;

#include "mytest.h"

MyTest mytest;

#include <unistd.h>	// sleep



static void pair_test_raw(const char *module		= "raw pair"	);
static void pair_test_null(const char *module		= "null pair"	);
static void pair_test(const char *module		= "pair"	);
static void pair_test_expired(const char *module	= "expired pair", bool sl = false);



int main(int argc, char **argv){
	mytest.begin("Pair");

	pair_test_null();
	pair_test_raw();
	pair_test();
	pair_test_expired();

	return mytest.end();
}



static void pair_test_raw_do(const char *module, const Pair & p, const StringRef &key, const StringRef &val){
	mytest("valid",		p.valid()			);

	mytest("key",		key == p.getKey()		);
	mytest("val",		val == p.getVal()		);

	mytest("cmp key",	p.cmp(key) == 0			);
	mytest("cmp",		p.cmp("~~~ non existent") < 0	);
	mytest("cmp",		p.cmp("!!! non existent") > 0	);

	Pair p2 = p;
	p2 = p;
}

static void pair_blob_test(const char *module, const PairBlob & p, const StringRef &key, const StringRef &val){
	mytest("valid",		p.valid()			);

	mytest("key",		key == p.getKey()		);
	mytest("val",		val == p.getVal()		);

	mytest("cmp key",	p.cmp(key.data()) == 0		);
	mytest("cmp",		p.cmp("~~~ non existent") < 0	);
	mytest("cmp",		p.cmp("!!! non existent") > 0	);
}

static void pair_test_raw(const char *module){
	const char *key = "name";
	const char *val = "Peter";

	static char raw_memory[] = {
		0x50, 0x00, 0x00, 0x00,		// created, 2012-07-13 11:01:20
		0x00, 0x00, 0x00, 0x00,		// milliseconds
		0x00, 0x00, 0x00, 0x00,		// expires
		0x00, 0x00, 0x00, 0x05,		// vallen
		0x00, 0x04,			// keylen
		'n', 'a', 'm', 'e', '\0',	// key
		'P', 'e', 't', 'e', 'r', '\0'	// val
	};

	auto pp = (const PairBlob *) raw_memory;
	pair_blob_test("pair::blob", *pp, key, val);

	const Pair p = (const PairBlob *) raw_memory;
	pair_test_raw_do(module, p, key, val);
}



static void pair_test_null(const char *module){
	const Pair p;

	mytest("null bool",	p == false			);
	mytest("null isTombstone",	p.isTombstone()			);
	mytest("null key",		p.getKey().empty()		);
	mytest("null val",		p.getVal().empty()		);
	mytest("null cmp",		p.cmp("bla") == 1		);
}



static void pair_test(const char *module){
	const char *key = "abcdef";
	const char *val = "1234567890";

	const Pair t = Pair::tombstone(key);

	mytest("null bool tomb",	t == true			);
	mytest("tombstone",		t.isTombstone()			);

	const Pair p = { key, val };

	mytest("null bool",		p == true			);

	mytest("key",			p.getKey() == key		);
	mytest("val",			p.getVal() == val		);

	mytest("cmp",			p.cmp(key) == 0			);
	mytest("cmp",			p.cmp("~~~ non existent") < 0	);
	mytest("cmp",			p.cmp("!!! non existent") > 0	);

	mytest("cmp",			p.cmp(t) == 0			);

	mytest("cmp null",		p.cmp((char *) nullptr)		);

	mytest("eq",			p.equals(key)			);
	mytest("!eq",			! p.equals("something")		);

	mytest("valid",			p.valid()			);
	mytest("valid",			p.valid(t)			);

	const Pair p2 = { "__smaller", "val"};

	mytest("cmp pair",		p.cmp(p) == 0			);
	mytest("cmp pair",		p2.cmp(p) < 0			);
	mytest("cmp pair",		p.cmp(p2) > 0			);

	mytest("eq pair",		p.equals(p)			);
	mytest("eq pair",		! p.equals(p2)			);
	mytest("!eq pair",		! p2.equals(p)			);

	{
		Pair m1 = { key, val };

		const Pair m2 = std::move(m1);
		mytest("move c-tor",		m2.getKey() == key		);

		const Pair m3 = m2;
		mytest("copy c-tor",		m2.getKey() == key		);
		mytest("copy c-tor",		m3.getKey() == key		);

		m1 = m2;
		mytest("copy assign",		m1.getKey() == key		);
	}
}



static void pair_test_expired(const char *module, bool sl){
	const Pair p1 = { "key", "val", 1 };

	mytest("not expired",	p1.valid()			);

	if (sl){
		printf("sleep for 2 sec...\n");
		sleep(2);
		mytest("expired",		! p1.valid()			);
	}

	const Pair p2 = { "key", "val", 1, 3600 * 24 /* 1970-01-02 */ };
	mytest("expired",		! p2.valid()				);
}


