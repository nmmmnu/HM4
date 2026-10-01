#include "ilist.h"

using hm4::Pair;
using hm4::OPair;

#include "mytest.h"

MyTest mytest;

#include <iterator>	// std::distance

#include "pmallocator.h"
#include "trackingallocator.h"
#include "stdallocator.h"

#include "htimerlist.h"

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
size_t listInsert(List &list, std::string_view key, std::string_view value){
	auto const &[ok, status, pair] = insert(list, key, value);

	// collect size, but via iterator...
	if (pair)
		return pair->bytes();
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
bool iteratorDereference(Iterator const &it, Iterator const &et, std::string_view value){
	return
		it != et &&
		  it->getVal() == value &&
		(*it).getVal() == value
	;
}

template <bool Exact, class List>
bool getCheck(List const &list, std::string_view key, std::string_view value){
	if constexpr(Exact){
		const auto *p = hm4::getPair(list, key);

		if (!value.empty())
			return p && p->getVal() == value;

		if (!p)
			return true;

		return ! p->isOK();
	}else{
		auto const it = list.find(key);
		auto const et = list.end();

		if (!value.empty())
			return iteratorDereference(it, et, value);

		if (it == et)
			return true;

		return ! it->isOK();
	}
}

// ==============================

template <class List>
void iterator_test_get(const List &list){
	mytest("it",			getCheck<0>(list, "1",			"Niki"	));
	mytest("it",			getCheck<1>(list, "1 firstname",	"Niki"	));
	mytest("it",			getCheck<0>(list, "2",			"22"	));
	mytest("it",			getCheck<1>(list, "2 age",		"22"	));
	mytest("it",			getCheck<0>(list, "3",			"Sofia"	));
	mytest("it",			getCheck<1>(list, "3 city",		"Sofia"	));
	mytest("it",			getCheck<0>(list, "4",			"Linux"	));
	mytest("it",			getCheck<1>(list, "4 os",		"Linux"	));
	mytest("it",			getCheck<1>(list, "4 osX",		{}	));
	mytest("it",			getCheck<1>(list, "5",			{}	));
	mytest("it",			getCheck<1>(list, "6",			{}	));

	// this is no longer supported
	//mytest("it", 			getCheck(list, "",		"Niki",		std::false_type{}	));
}

// ==============================

template<bool Reverse, typename It>
void iterator_test_(It it, It const et){
	const char *label_it_deref;
	const char *label_it_end;

	if constexpr(!Reverse){
		label_it_deref	= "it deref";
		label_it_end	= "*it end()";
	}else{
		label_it_deref	= "it reverse deref";
		label_it_end	= "*it reverse end()";
	}

	auto advance = [&it, &et, label_it_deref](const char *value){
		mytest(label_it_deref, 	iteratorDereference(it, et, value)	);

		++it;
	};

	if constexpr(!Reverse){
		advance("Niki"	);
		advance("22"	);
		advance("Sofia"	);
		advance("Linux"	);
	}else{
		advance("Linux"	);
		advance("Sofia"	);
		advance("22"	);
		advance("Niki"	);
	}

	mytest(label_it_end,		it == et				);
}

template <class List>
void iterator_test(List const &list){
	using ITC = typename std::iterator_traits<typename List::iterator>::iterator_category;

	if constexpr(true){ // for symetry
		iterator_test_<0>(
			std::begin(list),
			std::end(list)
		);
	}

	if constexpr(std::is_same_v<ITC, std::bidirectional_iterator_tag>){
		iterator_test_<1>(
			std::make_reverse_iterator(std::end(list)),
			std::make_reverse_iterator(std::begin(list))
		);

		iterator_test_<1>(
			std::rbegin(list),
			std::rend(list)
		);
	}
}

// ==============================

template<class List>
void iterator_test_empty_it(List const &list){
	using ITC = typename std::iterator_traits<typename List::iterator>::iterator_category;

	if constexpr(std::is_same_v<ITC, std::forward_iterator_tag>){ // for symetry
		mytest("empty it",		std::begin(list) == std::end(list)		);
	}

	if constexpr(std::is_same_v<ITC, std::bidirectional_iterator_tag>){
		auto _ = [](auto it){
			return std::make_reverse_iterator(it);
		};

		mytest("empty reverse it",	_(std::end(list)) == _(std::begin(list))	);
	}
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

	mytest("get",			getCheck<1>(list, "3 city",		"Sofia"	));
	mytest("get non existent",	getCheck<1>(list, "nonexistent",	{}	));


	// OVERWRITE

	const char *key_over = "2 val";
	const char *val_over = "overwritten";

	insert(list, key_over, "original"	);
	insert(list, key_over, val_over		);

	mytest("overwrite",		getCheck<1>(list, key_over,	val_over	));

	insert(list, key_over, "original", 0, 1	);

	mytest("overwrite 2",		getCheck<1>(list, key_over,	val_over	));

	// INSERT BY CLONING PAIR

	{
		OPair const p = Pair::create("clone_pair", "123");
		insert(list, *p);
	}

	mytest("clone",			getCheck<1>(list, "clone_pair",	"123"	));

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

	mytest("overwrite",		getCheck<1>(list, "3 city",	"Sofia"	));
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
	iterator_test_empty_it(list);

	// MOVE C-TOR

	listPopulate(list);

	List const mlist = std::move(list);
	mytest("move c-tor",		mlist.bytes() == bytes				);
}

template <template<class> class List>
void list_test_hint(std::string_view name){
	mytest.begin(name);

	Allocator_::v allocator;

	List<Allocator> list{ allocator };

	const char *key = "key";

	insert(list, key, "1234567890");

	auto const used = allocator.getUsedMemory();

	auto f = [&](std::string_view val1, std::string_view val2){

		auto chk = [&](std::string_view val){
			return
				getCheck<1>(list, key, val) &&
				used == allocator.getUsedMemory();
		};

		erase (list, key	);	mytest("hint test del",		chk({}		));
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

template <class List>
void list_test_blackhole(std::string_view name, List &list){
	mytest.begin(name);

	listPopulate(list);

	mytest("size estimated",	list.size() == 0						);
	mytest("size exact",		size(list) == 0							);
	mytest("size empty",		empty(list)							);
	mytest("size std::distance",	std::distance(std::begin(list), std::end(list)) == 0		);
	mytest("sizeof",		list.bytes() == 0						);

	mytest("put",			insert(list, "key", "val").ok					);
	mytest("find",			list.find   ("key") == std::end(list)				);
	mytest("find",			hm4::getPair(list, "key") == nullptr			);
	mytest("remove",		erase(list, "key").status == hm4::InsertResult::SKIP_DELETED	);
}

template <class List>
void list_test(std::string_view name, List &list){
	mytest.begin(name);

	return list_test(list);
}



template <class List, class ...Args>
void list_test(std::string_view name, Args &&...args){
	List list{ std::forward<Args>(args)... };

	hm4::HTimerList<List> listH(list);

	if constexpr(std::is_same_v<List, hm4::BlackHoleList>){
		list_test_blackhole(name, listH);
	}else{
		list_test(name, listH);
	}

	listH.chain(hm4::chain::log_histogram{});
}

#include "multi/duallist.h"
#include "vectorlist.h"

template<class Allocator>
using MyDualList = hm4::multi::DualList<hm4::VectorList<Allocator>, hm4::BlackHoleList, hm4::multi::DualListEraseType::NORMAL>;

template <>
void list_test<MyDualList<Allocator> >(std::string_view name){
	hm4::VectorList<Allocator>	memtable{ allocator };
	hm4::BlackHoleList		disktable;

	MyDualList<Allocator> list{ memtable, disktable };

	return list_test(name, list);
}

#include "multi/singlelist.h"

template<class Allocator>
using MySingleList = hm4::multi::SingleList<hm4::VectorList<Allocator> >;

template <>
void list_test<MySingleList<Allocator> >(std::string_view name){
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


