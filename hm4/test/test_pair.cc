#include "pair.h"

#include "pmallocator.h"
#include "trackingallocator.h"
#include "mallocallocator.h"

#include "mytest.h"

MyTest mytest;

#include <unistd.h>	// sleep



using hm4::Pair;
using hm4::OPair;



namespace{
	void pair_test_tombstone();
	void pair_test_raw();
	void pair_test_expired();
	void pair_test();
	void pair_test_ctor();
	void pair_test_allocator();
	void pair_size_test(size_t step);
}



int main(){
	pair_test_tombstone();
	pair_test_raw();
	pair_test();
	pair_test_expired();
	pair_test_ctor();
	pair_test_allocator();
	pair_size_test(7);

	//printf("Size: %zu bytes\n", sizeof(OPair));

	return mytest.end();
}



namespace{

	void pair_test_tombstone(){
		mytest.begin("Pair Tombstone");

		std::string_view const key = "name";

		const OPair p = Pair::create(key, Pair::TOMBSTONE);

		mytest("tombstone isTombstone",	p->isTombstone()	);
		mytest("tombstone key",		p->getKey() == key	);
		mytest("tombstone val",		p->getVal().empty()	);
	}

	const Pair *raw_memory(){
		static const uint8_t blob[] = {
			0x50, 0x00, 0x00, 0x00,		// created, 2012-07-13 11:01:20
			0x00, 0x00, 0x00, 0x00,		// milliseconds
			0x00, 0x00, 0x00, 0x00,		// expires
			0x00, 0x04,			// keylen
			0x00, 0x05,			// vallen
			'n', 'a', 'm', 'e', '\0',	// key
			'P', 'e', 't', 'e', 'r', '\0'	// val
		};

		return reinterpret_cast<const Pair *>(blob);
	}

	void pair_test_raw(){
		mytest.begin("Pair Raw");

		std::string_view const key = "name";
		std::string_view const val = "Peter";

		const Pair *p = raw_memory();

		mytest("valid",		p->isValid()			);

		mytest("key",		p->getKey() == key		);
		mytest("val",		p->getVal() == val		);

		mytest("cmp key",	p->cmp(key) == 0		);
		mytest("cmp",		p->cmp("~~~ non existent") < 0	);
		mytest("cmp",		p->cmp("!!! non existent") > 0	);
	}

	void pair_test_expired(){
		mytest.begin("Pair Expired");

		const OPair p1 = Pair::create( "key", "val", 1 );

		mytest("not expired",	p1->isValid()				);

		puts("sleep for 2 sec...\n");
		sleep(2);
		mytest("expired",	! p1->isValid()				);

		const OPair p2 = Pair::create( "key", "val", 1, 3600 * 24 /* 1970-01-02 */ );
		mytest("expired",		! p2->isValid()			);
	}

	void pair_test_ctor(){
		mytest.begin("c-tor / d-tor");

		const OPair a = Pair::create( "1", "one" );

		const OPair b = Pair::clone( *a );
		mytest("copy Pair",		b->getKey() == "1"		);
	}

	void pair_test_allocator(){
		mytest.begin("allocator");

		using namespace MyAllocator;
		using Allocator = PMOwnerAllocator<TrackingAllocator<MallocAllocator> >;

		Allocator a;

		const auto p = Pair::smart_ptr::create( a, "key", "val" );

		p->print();
	}

	void pair_test(){
		mytest.begin("Pair");

		std::string_view const key = "abcdef";
		std::string_view const val = "1234567890";

		const OPair t = Pair::create(key, Pair::TOMBSTONE, 0, 3600 * 24 /* 1970-01-02 */ );

		const OPair p = Pair::create( key, val );

		mytest("key",			p->getKey() == key		);
		mytest("val",			p->getVal() == val		);

		mytest("cmp",			p->cmp(key) == 0		);
		mytest("cmp",			p->cmp("~~~ non existent") < 0	);
		mytest("cmp",			p->cmp("!!! non existent") > 0	);

		mytest("cmp",			p->cmp(t->getKey()) == 0	);

		mytest("eq",			p->equals(key)			);
		mytest("!eq",			! p->equals("something")	);

		mytest("valid",			p->isValid()			);
		mytest("valid",			p->isValidForReplace(*t)	);

		mytest("valid",			t->isValid()			);
		mytest("valid",			t->isValid(std::false_type{})	);
		mytest("valid",			! t->isValid(std::true_type{})	);

		mytest("valid",			! t->isValidForReplace(*p)	);
	}

	[[maybe_unused]]
	void pair_size_test(size_t const step){
		mytest.begin("Pair Size (slow)");

		auto f = [](const char *msg, size_t i, std::string_view x, auto const &p){
			if constexpr(0){
				printf("%zu | %s\n", i, x.data());

				print(p);
			}else{
				printf("%zu\n", i);
			}

			mytest(msg, false);
		};

		std::string_view const key = "k";

		std::string master_val( hm4::PairConf::MAX_VAL_SIZE, '*' );

		for(size_t i = 0; i < hm4::PairConf::MAX_VAL_SIZE; i += step){
			std::string_view const val{ master_val.data(), i };

			const OPair p = Pair::create( key, val );

		//	printf("%10zu | %10zu | %10zu\n", i, p->getKey().size(), p->getVal().size() );

			if (p->getKey() != key)
				f("size error key ", i, key, p);

			if (p->getVal() != val)
				f("size error val ", i, val, p);

			if (i == 0){
				if (!p->isTombstone())
					f("size error tomb zero ", i, val, p);
			}else{
				if (p->isTombstone())
					f("size error tomb", i, val, p);
			}
		}
	}

} // anonymous namespace

