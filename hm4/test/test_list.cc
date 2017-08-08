#include "pair.h"

using Pair	= hm4::Pair;

#include "mytest.h"

MyTest mytest;

#include <utility>	// std::pair



template <class LIST>
size_t list_insert(LIST &list, Pair &&p){
	const size_t size = p.bytes();
	list.insert(std::move(p));
	return size;
}

template <class LIST>
auto list_populate(LIST &list){
	list.clear();

	size_t size = 0;

	size += list_insert(list, {"3 city",	"Sofia"	});
	size += list_insert(list, {"1 name",	"Niki"	});
	size += list_insert(list, {"4 os",	"Linux"	});
	size += list_insert(list, {"2 age",	"22"	});

	return std::make_pair( (typename LIST::size_type) 4, size);
}

template <class IT>
void iterator_deref(const IT &it, const IT &et, const char *value){
	mytest("*it   deref",	it != et && (*it).getVal() == value	);
	mytest(" it-> deref",	it != et &&   it->getVal() == value	);
}

template <class IT>
void iterator_advance(IT &it, const IT &et, const char *value){
	iterator_deref(it, et, value);
	++it;
}

template <class LIST>
void iterator_test(const LIST &list){
	auto it = list.begin();
	auto et = list.end();

	iterator_advance(it, et, "Niki"		);
	iterator_advance(it, et, "22"		);
	iterator_advance(it, et, "Sofia"	);
	iterator_advance(it, et, "Linux"	);

	mytest("*it end()",	it == et				);
}

template <class LIST>
void list_lowerBound(const LIST &list, const char *key, const char *value){
	auto it = list.lowerBound(key);
	auto et = list.end();

	if (value){

	iterator_advance(it, et, value);

	}else{

	mytest("it == et",		it == et			);

	}
}

template <class LIST>
void iterator_get_test(const LIST &list){
	list_lowerBound(list, "1",	"Niki"	);
	list_lowerBound(list, "1 name",	"Niki"	);
	list_lowerBound(list, "2",	"22"	);
	list_lowerBound(list, "2 age",	"22"	);
	list_lowerBound(list, "3",	"Sofia"	);
	list_lowerBound(list, "3 city",	"Sofia"	);
	list_lowerBound(list, "4",	"Linux"	);
	list_lowerBound(list, "4 os",	"Linux"	);
	list_lowerBound(list, "4 osX",	nullptr	);
	list_lowerBound(list, "5",	nullptr	);
	list_lowerBound(list, "6",	nullptr	);

	list_lowerBound(list, "",	"Niki"	);
}

template <class LIST>
void list_test(LIST &list){
	// GENERAL

	const auto a = list_populate(list);

	auto const count = a.first;
	auto const size  = a.second;

	mytest("count",			list.size() == count		);
	mytest("count estim",		list.size(true) >= count	);
	mytest("empty",			! list.empty()			);
	mytest("sizeof",		list.bytes() == size		);


	Pair p;



	// GET

	p = list["3 city"];
	mytest("get",			p.getVal() == "Sofia"		);

	p = list["nonexistent"];
	mytest("get non existent",	! p				);



	// OVERWRITE

	const char *key_over = "2 val";
	const char *val_over = "overwritten";

	list.insert( Pair(key_over, "original") );
	list.insert( Pair(key_over, val_over) );
	p = list[key_over];
	mytest("overwrite",		p.getVal() == val_over		);



	// ERASE

	list_populate(list);

	// remove non existent
	list.erase("nonexistent");

	// remove middle
	list.erase("2 age");

	// remove first
	list.erase("1 name");

	// remove last
	list.erase("4 os");

	p = list["3 city"];

	mytest("remove",		p.getVal() == "Sofia"		);
	mytest("remove count",		list.size() == 1		);
	mytest("remove sizeof",		list.bytes() == p.bytes()	);

	// overwrite sizeof test
	const Pair sopair = { "3 city", "" };

	list.insert(sopair);
	mytest("overwrite count",	list.size() == 1		);
	mytest("overwrite sizeof",	list.bytes() == sopair.bytes()	);

	// remove last element
	list.erase("3 city");

	// remove non existent from empty list
	list.erase("nonexistent");

	mytest("remove count",		list.size() == 0		);
	mytest("remove empty",		list.empty()			);



	// ITERATOR

	list_populate(list);

	iterator_test(list);
	iterator_get_test(list);

	list.clear();
	mytest("empty iterator",	list.begin() == list.end()	);



	// MOVE

	list_populate(list);

	LIST mlist = std::move(list);
	mytest("move c-tor",		mlist.bytes() == size		);

	// checking if list.empty() == true
	// is wrong and should not be tested,
	// because it could be done via copy c-tor or swap
}

#include "skiplist.h"

static void skiplist_lanes_test() __attribute__((unused));

static void skiplist_lanes_test(){
	hm4::SkipList list;

	list.emplace("name",		"Niki"		);
	list.emplace("city",		"Sofia"		);
	list.emplace("state",		"na"		);
	list.emplace("zip",		"1000"		);
	list.emplace("country",		"BG"		);
	list.emplace("phone",		"+358 888 1000"	);
	list.emplace("fax",		"+358 888 2000"	);
	list.emplace("email",		"user@aol.com"	);
	list.emplace("laptop",		"Dell"		);
	list.emplace("os",		"Archlinux"	);
	list.emplace("mouse",		"Logitech"	);

	list.printLanes();
}

#include "blackholelist.h"

template<>
void list_test(hm4::BlackHoleList &list){
	Pair p = nullptr;

	list_populate(list);

	mytest("count",			list.size() == 0			);
	mytest("count estim",		list.size(true) == 0			);
	mytest("empty",			list.empty()				);
	mytest("sizeof",		list.bytes() == 0			);

	mytest("put",			list.insert( { "key", "val" } )		);
	mytest("get",			! list["key"]				);
	mytest("remove",		list.erase("key")			);
}


template <class LIST>
void list_test(const char *name, LIST &list){
	mytest.begin(name);

	return list_test(list);
}


template <class LIST>
void list_test(const char *name){
	LIST list;

	return list_test(name, list);
}

#include "multi/duallist.h"

using MyDualList = hm4::multi::DualList<hm4::SkipList, hm4::BlackHoleList>;

template <>
void list_test<MyDualList>(const char *name){
	hm4::SkipList		memtable;
	hm4::BlackHoleList	disktable;
	MyDualList list{ memtable, disktable };

	return list_test(name, list);
}

#include "decoratorlist.h"

using MyDecoratorList = hm4::DecoratorList<hm4::SkipList>;

template <>
void list_test<MyDecoratorList>(const char *name){
	hm4::SkipList		memtable;
	MyDecoratorList list{ memtable };

	return list_test(name, list);
}

#include "vectorlist.h"
#include "linklist.h"

int main(int argc, char **argv){

	list_test<hm4::VectorList	>("VectorList"		);
	list_test<hm4::LinkList		>("LinkList"		);
	list_test<hm4::SkipList		>("SkipList"		);
	list_test<hm4::BlackHoleList	>("BlackHoleList"	);

	list_test<MyDecoratorList	>("DecoratorList"	);

	list_test<MyDualList		>("DualList"		);

//	skiplist_lanes_test();

	return mytest.end();
}


