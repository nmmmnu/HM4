#include "ilist.h"

using hm4::Pair;
using hm4::OPair;

#include "mytest.h"

MyTest mytest;

#include <iterator>	// std::distance

#include "pmallocator.h"
#include "trackingallocator.h"
#include "stdallocator.h"
#include "arenaallocator.h"

struct Allocator_1{
	using type	= MyAllocator::STDAllocator;
	using v		= type;
};

struct Allocator_2{
	using type	= MyAllocator::PMAllocator;
	using v		= MyAllocator::PMOwnerAllocator<MyAllocator::TrackingAllocator<MyAllocator::STDAllocator> >;
};

using Allocator_	= Allocator_1;

using Allocator		= Allocator_::type;

Allocator_::v allocator;

// ==============================

template <class List>
size_t listInsert(List &list, const char *key, const char *value){
	auto it = insert(list, key, value);

	// collect size, but via iterator...
	if (it != std::end(list))
		return it->bytes();
	else
		return 0;

	// collect size...
	// return Pair::bytes(key, value);
}

template <class List>
auto listPopulate(List &list){
	list.clear();

	size_t size =
		listInsert(list, "3 city",	"Sofia"	) +
		// this tests HPair::HKey as well:
		listInsert(list, "1 firstname",	"Niki"	) +
		listInsert(list, "4 os",	"Linux"	) +
		listInsert(list, "2 age",	"22"	) +
		0
	;

	return std::pair{
		(typename List::size_type) 4,
		size
	};
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

	if (it == et)
		return true;

	return ! it->isOK();
}

// ==============================

template <class List>
void iterator_test_get(const List &list){

	mytest("it", 			getCheck(list, "1",		"Niki",		std::false_type{}	));
	mytest("it", 			getCheck(list, "1 firstname",	"Niki",		std::true_type{}	));
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
	listPopulate(list);
	listPopulate(list);
	listPopulate(list);

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

	insert(list, key_over, "original"	);
	insert(list, key_over, val_over		);

	mytest("overwrite",		getCheck(list, key_over,	val_over,	std::true_type{}	));

	insert(list, key_over, "original", 0, 1	);

	mytest("overwrite 2",		getCheck(list, key_over,	val_over,	std::true_type{}	));

	// INSERT BY CLONING PAIR

	{
		OPair const p = Pair::create("clone_pair", "123");
		insert(list, *p);
	}

	mytest("clone",			getCheck(list, "clone_pair",	"123",		std::true_type{}	));

	// ERASE

	listPopulate(list);

	// remove non existent
	erase(list, "nonexistent");

	// remove middle
	erase(list, "2 age");

	// remove first
	erase(list, "1 firstname");

	// remove last
	erase(list, "4 os");

	mytest("overwrite",		getCheck(list, "3 city",	"Sofia",	std::true_type{}	));
	mytest("remove count",		list.size() == 1				);

	// remove last element
	erase(list, "3 city");

	// remove non existent from empty list
	erase(list, "nonexistent");

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

template <template<class> class List>
void list_test_hint(const char *name){
	mytest.begin(name);

	using Allocator = MyAllocator::ArenaAllocator;
	Allocator allocator{ 1u * 1024 * 1024 };

	List<Allocator> list{ allocator };

	const char *key = "key";

	insert(list, key, "1234567890");

	auto const used = allocator.getUsedMemory();

	auto f = [&](const char *val1, const char *val2){

		auto chk = [&](const char *val){
			return
				getCheck(list, key, val, std::true_type{}) &&
				used == allocator.getUsedMemory();
		};

		insert(list, key	);	mytest("hint test del",		chk(nullptr	));
		insert(list, key, val1	);	mytest("hint test set1",	chk(val1	));
		insert(list, key, val2	);	mytest("hint test set2",	chk(val2	));
	};

	insert(list, key, "123456789");

	f("12345678"	, "X2345678"	);
	f("1234567"	, "X234567"	);
	f("123456"	, "X23456"	);
	f("12345"	, "X2345"	);
	f("1234"	, "X234"	);
	f("123"		, "X23"		);
	f("12"		, "X2"		);
	f("1"		, "X"		);
}

#include "blackholelist.h"

template<>
void list_test(hm4::BlackHoleList &list){
	listPopulate(list);

	mytest("size estimated",	list.size() == 0					);
	mytest("size exact",		size(list) == 0						);
	mytest("size empty",		empty(list)						);
	mytest("size std::distance",	std::distance(std::begin(list), std::end(list)) == 0	);
	mytest("sizeof",		list.bytes() == 0					);

	mytest("put",			insert(list, "key", "val") == std::end(list)		);
	mytest("find",			list.find("key", std::false_type{}) == std::end(list)	);
	mytest("find",			list.find("key", std::true_type{} ) == std::end(list)	);
	mytest("remove",		erase(list, "key")					);
}

template <class List>
void list_test(const char *name, List &list){
	mytest.begin(name);

	return list_test(list);
}


template <class List, class ...Args>
void list_test(const char *name, Args &&...args){
	List list{ std::forward<Args>(args)... };

	return list_test(name, list);
}

#include "multi/duallist.h"
#include "vectorlist.h"

template<class Allocator>
using MyDualList = hm4::multi::DualList<hm4::VectorList<Allocator>, hm4::BlackHoleList, hm4::multi::DualListEraseType::NORMAL>;

template <>
void list_test<MyDualList<Allocator> >(const char *name){
	hm4::VectorList<Allocator>	memtable{ allocator };
	hm4::BlackHoleList		disktable;

	MyDualList<Allocator> list{ memtable, disktable };

	return list_test(name, list);
}

#include "multi/singlelist.h"

template<class Allocator>
using MySingleList = hm4::multi::SingleList<hm4::VectorList<Allocator> >;

template <>
void list_test<MySingleList<Allocator> >(const char *name){
	hm4::VectorList<Allocator>	memtable{ allocator };
	MySingleList<Allocator>		list{ memtable };

	return list_test(name, list);
}

#include "skiplist.h"
#include "unrolledskiplist.h"

[[maybe_unused]]
static void skiplist_lanes_test(){
	hm4::SkipList<Allocator> list{ allocator };

	insert(list, "name",	"Niki"		);
	insert(list, "city",	"Sofia"		);
	insert(list, "state",	"na"		);
	insert(list, "zip",	"1000"		);
	insert(list, "country",	"BG"		);
	insert(list, "phone",	"+358 888 1000"	);
	insert(list, "fax",	"+358 888 2000"	);
	insert(list, "email",	"user@aol.com"	);
	insert(list, "laptop",	"Dell"		);
	insert(list, "os",	"Archlinux"	);
	insert(list, "mouse",	"Logitech"	);

	list.printLanes();
}

// ==============================

#include "linklist.h"
#include "unrolledlinklist.h"
#include "avllist.h"

int main(){
	list_test<hm4::BlackHoleList			>("BlackHoleList"			);
	list_test<hm4::VectorList	<Allocator>	>("VectorList"		, allocator	);
	list_test<MyDualList		<Allocator>	>("DualList"				);
	list_test<MySingleList		<Allocator>	>("SingeList"				);

	list_test<hm4::LinkList		<Allocator>	>("LinkList"		, allocator	);
	list_test<hm4::SkipList		<Allocator>	>("SkipList"		, allocator	);

	list_test<hm4::UnrolledLinkList	<Allocator>	>("UnrolledLinkList"	, allocator	);
	list_test<hm4::UnrolledSkipList	<Allocator>	>("UnrolledSkipList"	, allocator	);

	list_test<hm4::AVLList		<Allocator>	>("AVLList"		, allocator	);

//	skiplist_lanes_test();

	list_test_hint<hm4::VectorList		>("VectorList"		);
	list_test_hint<hm4::LinkList		>("LinkList"		);
	list_test_hint<hm4::SkipList		>("SkipList"		);
	list_test_hint<hm4::UnrolledLinkList	>("UnrolledLinkList"	);
	list_test_hint<hm4::UnrolledSkipList	>("UnrolledSkipList"	);
	list_test_hint<hm4::AVLList		>("AVLList"		);

	return mytest.end();
}


