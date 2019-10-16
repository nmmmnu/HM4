#include "ilist.h"

using hm4::Pair;
using hm4::OPair;

#include "mytest.h"

MyTest mytest;

#include <iterator>	// std::distance

// ==============================

template <class List>
size_t listInsert(List &list, const char *key, const char *value){
	OPair p = { key, value };

	size_t const size = p->bytes();
	list.insert(std::move(p));
	return size;
}

template <class List>
auto listPopulate(List &list){
	list.clear();

	size_t size =
		listInsert(list, "3 city",	"Sofia"	) +
		listInsert(list, "1 name",	"Niki"	) +
		listInsert(list, "4 os",	"Linux"	) +
		listInsert(list, "2 age",	"22"	) +
		0
	;

	return std::make_pair( (typename List::size_type) 4, size);
}

// ==============================

template <class Iterator>
bool iteratorDereference(Iterator const &it, Iterator const &et, const char *value){
	return
		it != et &&
		  it->getVal() == value &&
		(*it).getVal() == value
	;
}

template <class List, bool B>
bool getCheck(List const &list, const char *key, const char *value, std::bool_constant<B> const exact){
	auto const it = list.find(key, exact);
	auto const et = list.end();

	if (value)
		return iteratorDereference(it, et, value);

	return it == et;
}

// ==============================

template <class List>
void iterator_test_get(const List &list){

	mytest("it", 			getCheck(list, "1",		"Niki",		std::false_type{}	));
	mytest("it", 			getCheck(list, "1 name",	"Niki",		std::true_type{}	));
	mytest("it", 			getCheck(list, "2",		"22",		std::false_type{}	));
	mytest("it", 			getCheck(list, "2 age",		"22",		std::true_type{}	));
	mytest("it", 			getCheck(list, "3",		"Sofia",	std::false_type{}	));
	mytest("it", 			getCheck(list, "3 city",	"Sofia",	std::true_type{}	));
	mytest("it", 			getCheck(list, "4",		"Linux",	std::false_type{}	));
	mytest("it", 			getCheck(list, "4 os",		"Linux",	std::true_type{}	));
	mytest("it", 			getCheck(list, "4 osX",		nullptr,	std::true_type{}	));
	mytest("it", 			getCheck(list, "5",		nullptr,	std::true_type{}	));
	mytest("it", 			getCheck(list, "6",		nullptr,	std::true_type{}	));

	// this is no longer supported
	//mytest("it", 			getCheck(list, "",		"Niki",		std::false_type{}	));
}

template <class List>
void iterator_test(List const &list){
	auto       it = std::begin(list);
	auto const et = std::end(list);

	auto advance = [&it, &et](const char *value){
		mytest("it deref", 	iteratorDereference(it, et, value)	);
		++it;
	};

	advance("Niki"	);
	advance("22"	);
	advance("Sofia"	);
	advance("Linux"	);

	mytest("*it end()",		it == et				);
}

// ==============================

template <class List>
void list_test(List &list){
	// GENERAL

	const auto&[ count, bytes ] = listPopulate(list);

	mytest("size estimated",	list.size() >= count				);
	mytest("size exact",		size(list) == count				);
	mytest("size empty",		! empty(list)					);
	mytest("size std::distance",	static_cast<typename List::size_type>(
						std::distance(std::begin(list), std::end(list))
					) == count					);
	mytest("sizeof",		list.bytes() == bytes				);


	// GET

	mytest("get",			getCheck(list, "3 city",	"Sofia",	std::true_type{}	));
	mytest("get non existent",	getCheck(list, "nonexistent",	nullptr,	std::true_type{}	));


	// OVERWRITE

	const char *key_over = "2 val";
	const char *val_over = "overwritten";

	list.insert( { key_over, "original"	} );
	list.insert( { key_over, val_over	} );

	mytest("overwrite",		getCheck(list, key_over,	val_over,	std::true_type{}	));


	// ERASE

	listPopulate(list);

	// remove non existent
	list.erase("nonexistent");

	// remove middle
	list.erase("2 age");

	// remove first
	list.erase("1 name");

	// remove last
	list.erase("4 os");

	mytest("overwrite",		getCheck(list, "3 city",	"Sofia",	std::true_type{}	));
	mytest("remove count",		list.size() == 1				);

	// remove last element
	list.erase("3 city");

	// remove non existent from empty list
	list.erase("nonexistent");

	mytest("remove count",		list.size() == 0				);
	mytest("remove empty",		empty(list)					);


	// ITERATOR

	listPopulate(list);

	iterator_test(list);
	iterator_test_get(list);

	list.clear();
	mytest("empty iterator",	std::begin(list) == std::end(list)		);


	// MOVE C-TOR

	listPopulate(list);

	List const mlist = std::move(list);
	mytest("move c-tor",		mlist.bytes() == bytes				);
}

#include "blackholelist.h"

template<>
void list_test(hm4::BlackHoleList &list){
	listPopulate(list);

	mytest("size estimated",	list.size() == 0				);
	mytest("size exact",		size(list) == 0					);
	mytest("size empty",		empty(list)					);
	mytest("size std::distance",	std::distance(std::begin(list), std::end(list)) == 0	);
	mytest("sizeof",		list.bytes() == 0				);

	mytest("put",			list.insert( { "key", "val" } )			);
	mytest("find",			list.find("key", std::true_type{}) == std::end(list)	);
	mytest("remove",		list.erase("key")				);
}

// ==============================

template <class List>
void list_test(const char *name, List &list){
	mytest.begin(name);

	return list_test(list);
}

// ==============================

template <class List>
void list_test(const char *name){
	List list;

	return list_test(name, list);
}

#include "multi/duallist.h"
#include "skiplist.h"

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

// ==============================

static void skiplist_lanes_test() __attribute__((unused));

static void skiplist_lanes_test(){
	hm4::SkipList list;

	list.insert( { "name",		"Niki"		} );
	list.insert( { "city",		"Sofia"		} );
	list.insert( { "state",		"na"		} );
	list.insert( { "zip",		"1000"		} );
	list.insert( { "country",	"BG"		} );
	list.insert( { "phone",		"+358 888 1000"	} );
	list.insert( { "fax",		"+358 888 2000"	} );
	list.insert( { "email",		"user@aol.com"	} );
	list.insert( { "laptop",	"Dell"		} );
	list.insert( { "os",		"Archlinux"	} );
	list.insert( { "mouse",		"Logitech"	} );

	list.printLanes();
}

// ==============================

#include "vectorlist.h"
#include "linklist.h"

int main(){
	list_test<hm4::VectorList	>("VectorList"		);
	list_test<hm4::LinkList		>("LinkList"		);
	list_test<hm4::SkipList		>("SkipList"		);
	list_test<hm4::BlackHoleList	>("BlackHoleList"	);
	list_test<MyDualList		>("DualList"		);
	list_test<MyDecoratorList	>("DecoratorList"	);


//	skiplist_lanes_test();

	return mytest.end();
}


