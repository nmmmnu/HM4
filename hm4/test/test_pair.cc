#include "pair.h"

using hm4::Pair;
using hm4::OPair;

#include "mytest.h"

MyTest mytest;

#include <unistd.h>	// sleep



static void pair_test_tombstone();
static void pair_test_raw();
static void pair_test_expired(bool slow = false);
static void pair_test();


static void pair_test_ctor();


int main(){
	pair_test_tombstone();
	pair_test_raw();
	pair_test();
	pair_test_expired();
	pair_test_ctor();

	return mytest.end();
}

// ===================================

static void pair_test_tombstone(){
	mytest.begin("Pair Tombstone");

	const char *key = "name";

	const auto p = OPair::tombstone(key);

	mytest("tombstone bool",	p			);
	mytest("tombstone isTombstone",	p->isTombstone()	);
	mytest("tombstone key",		p->getKey() == key	);
	mytest("tombstone val",		p->getVal().empty()	);
}

// ===================================

static const Pair *raw_memory(){
	static const char blob[] = {
		0x50, 0x00, 0x00, 0x00,		// created, 2012-07-13 11:01:20
		0x00, 0x00, 0x00, 0x00,		// milliseconds
		0x00, 0x00, 0x00, 0x00,		// expires
		0x00, 0x00, 0x00, 0x05,		// vallen
		0x00, 0x04,			// keylen
		'n', 'a', 'm', 'e', '\0',	// key
		'P', 'e', 't', 'e', 'r', '\0'	// val
	};

	return reinterpret_cast<const Pair *>(blob);
}

static void pair_test_raw(){
	mytest.begin("Pair Raw");

	const char *key = "name";
	const char *val = "Peter";

	const Pair *p = raw_memory();

	mytest("valid",		p->isValid()			);

	mytest("key",		p->getKey() == key		);
	mytest("val",		p->getVal() == val		);

	mytest("cmp key",	p->cmp(key) == 0		);
	mytest("cmp",		p->cmp("~~~ non existent") < 0	);
	mytest("cmp",		p->cmp("!!! non existent") > 0	);
}

// ===================================

static void pair_test_expired(bool const slow){
	mytest.begin("Pair Expired");

	const OPair p1 = { "key", "val", 1 };

	mytest("not expired",	p1->isValid()				);

	if (slow){
		printf("sleep for 2 sec...\n");
		sleep(2);
		mytest("expired",	! p1->isValid()			);
	}

	const OPair p2 = { "key", "val", 1, 3600 * 24 /* 1970-01-02 */ };
	mytest("expired",		! p2->isValid()			);
}

// ===================================

static void pair_test_ctor(){
	mytest.begin("c-tor / d-tor");

	const OPair o1 = { "1", "one" };
	const OPair o2 = { "2", "two" };
	const OPair o3 = { "3", "tri" };

	OPair p1 = *o1;
	mytest("copy Pair",		p1->getKey() == "1"		);

	OPair p2 = o2;
	mytest("copy c-tor",		p2->getKey() == "2"		);

	p1 = *o3;
	mytest("assign Pair",		p1->getKey() == "3"		);

	p1 = p2;
	mytest("copy assign",		p1->getKey() == "2"		);

	p1 = o1;
	p2 = o2;

	p1.swap(p2);
	mytest("swap",			p1->getKey() == "2"		);
	mytest("swap",			p2->getKey() == "1"		);

	OPair p3 = std::move(p1);
	mytest("move c-tor",		p3->getKey() == "2"		);

	p3 = std::move(p2);
	mytest("move assign",		p3->getKey() == "1"		);

}

// ===================================

static void pair_test(){
	mytest.begin("Pair");

	const char *key = "abcdef";
	const char *val = "1234567890";

	const OPair t = OPair::tombstone(key);

	const OPair p = { key, val };

	mytest("null bool",		p == true			);

	mytest("key",			p->getKey() == key		);
	mytest("val",			p->getVal() == val		);

	mytest("cmp",			p->cmp(key) == 0		);
	mytest("cmp",			p->cmp("~~~ non existent") < 0	);
	mytest("cmp",			p->cmp("!!! non existent") > 0	);

	mytest("cmp",			p->cmp(t->getKey()) == 0	);

	mytest("eq",			p->equals(key)			);
	mytest("!eq",			! p->equals("something")	);

	mytest("valid",			p->isValid()			);
	mytest("valid",			p->isValid(t)			);
}

