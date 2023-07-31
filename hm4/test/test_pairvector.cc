#include "pairvector.h"
#include "ilist.h"

#include "pmallocator.h"
#include "trackingallocator.h"
#include "stdallocator.h"

#include "mytest.h"



struct Allocator_1{
	using type	= MyAllocator::STDAllocator;
	using v		= type;
};

struct Allocator_2{
	using type	= MyAllocator::PMAllocator;
	using v		= MyAllocator::PMOwnerAllocator<MyAllocator::TrackingAllocator<MyAllocator::STDAllocator> >;
};

using Allocator_	= Allocator_2;

using Allocator		= Allocator_::type;



using MyPairVector = hm4::PairVector<Allocator_::type, 32>;



template<class V, typename ...Args>
void insert(V &v, Allocator &allocator, Args &&...args){
	hm4::PairFactory::Normal f{ std::forward<Args>(args)... };

	v.insertF(f, allocator, nullptr);
}

template<class V>
void print(V &v){
	if (v.size() == 0){
		printf("--- list is empty ---\n");
	}

	for(auto &x : v)
		x.print();
}


MyTest mytest;

void test_pairvector(){
	mytest.begin("PairVector");

	Allocator_::v allocator;

	MyPairVector v;

	print(v);

	mytest("insert",	true									);

	insert(v, allocator, "11 mouse"		, "Logitech"		);
	insert(v, allocator, "10 os"		, "Archlinux"		);
	insert(v, allocator, "09 laptop"	, "Dell"		);
	insert(v, allocator, "01 name"		, "Niki"		);
	insert(v, allocator, "02 city"		, "Sofia"		);
	insert(v, allocator, "03 state"		, "na"			);
	insert(v, allocator, "04 zip"		, "1000"		);
	insert(v, allocator, "05 country"	, "BG"			);
	insert(v, allocator, "06 phone"		, "+358 888 1000"	);
	insert(v, allocator, "07 fax"		, "+358 888 2000"	);
	insert(v, allocator, "08 email"		, "user@aol.com"	);

	print(v);

	mytest("size",		v.size() == 11								);
	mytest("front",		v.front().getVal() == "Niki"						);
	mytest("back",		v.back() .getVal()  == "Logitech"					);

	mytest("search",	v.find("04",		std::true_type{}	) == std::end(v)	);
	mytest("search",	v.find("04",		std::false_type{}	)->getVal() == "1000"	);
	mytest("search",	v.find("04 zip",	std::true_type{}	)->getVal() == "1000"	);

	v.erase_("04 zip__",	allocator);
	v.erase_("04 zip",	allocator);

	mytest("erase size",	v.size()  == 10				);

	v.erase_("04 zip",	allocator);
	v.erase_("03 state__",	allocator);
	v.erase_("03 state",	allocator);
	v.erase_("03 state",	allocator);

	mytest("erase size",	v.size()  == 9				);

	insert(v, allocator, "07 fax"		, "+358 888 2000"	);
	insert(v, allocator, "08 email"		, "user@aol.com"	);

	mytest("replace size",	v.size()  == 9				);

	print(v);

	MyPairVector w;

	v.split(w);

	mytest("split size",	v.size()  == 4				);
	print(v);
	mytest("split size",	w.size()  == 5				);
	print(w);

	v.merge(w);

	mytest("merge size",	v.size()  == 9				);
	print(v);
	mytest("merge size",	w.size()  == 0				);
	print(w);

	v.destruct(allocator);
}

int main(){
	test_pairvector();

	return mytest.end();
}

