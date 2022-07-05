#include "pair.h"
#include "ilist.h"

#include <unistd.h> 	// usleep

#include <iostream>

using hm4::Pair;
using hm4::OPair;

#include "mytest.h"

MyTest mytest;

inline void sleep(){
	usleep(100);
}



#include "pmallocator.h"
#include "trackingallocator.h"
#include "stdallocator.h"

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



template <class Iterator>
bool iteratorDereference(Iterator const &it, Iterator const &et, const char *value){
	if (false){
		std::cout
			<< (it != et ? "*" : "end")	<< ' '
			<< value			<< '\n';

		if (it != et)
			print(*it);
	}

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

	return iteratorDereference(it, et, value);
}

template <class List>
bool getCheck(List const &list, const char *key){
	auto const it = list.find(key, std::true_type{});
	auto const et = list.end();

	return it == et;
}

// ==============================

template <class List>
void iterator_test_get(const List &list){
	constexpr std::false_type	N{};
	constexpr std::true_type	Y{};

	mytest("it", 			getCheck(list, "1",		"Niki",		N	));
	mytest("it", 			getCheck(list, "1 name",	"Niki",		Y	));
	mytest("it", 			getCheck(list, "2",		"22",		N	));
	mytest("it", 			getCheck(list, "2 age",		"22",		Y	));
	mytest("it", 			getCheck(list, "3",		"Sofia",	N	));
	mytest("it", 			getCheck(list, "3 city",	"Sofia",	Y	));
	mytest("it", 			getCheck(list, "4",		"Linux",	N	));
	mytest("it", 			getCheck(list, "4 os",		"Linux",	Y	));
	mytest("it", 			getCheck(list, "4 osX"					));
	mytest("it", 			getCheck(list, "5"					));
	mytest("it", 			getCheck(list, "6"					));

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
void list_test(const List &list, typename List::size_type const count, size_t const bytes){

	// GENERAL

	mytest("size estimated",	list.size() >= count				);
//	mytest("size exact",		size(list) == count				);
	mytest("size empty",		! empty(list)					);
//	mytest("size std::distance",	static_cast<typename List::size_type>(
//						std::distance(std::begin(list), std::end(list))
//					) == count					);

	mytest("sizeof",		list.bytes() == bytes				);


	// GET

	mytest("get",			getCheck(list, "3 city",	"Sofia", std::true_type{}	));
	mytest("get non existent",	getCheck(list, "nonexistent"					));

	//std::cout << list.bytes() << ' ' << bytes << '\n';


	// ITERATOR

	iterator_test(list);
	iterator_test_get(list);


	// MOVE C-TOR

	List const mlist = std::move(list);
	mytest("move c-tor",		mlist.bytes() == bytes				);

}

// ==============================

#include "multi/duallist.h"

template <class List>
size_t listInsert(List &list, const char *key, const char *value){
	size_t const size = Pair::bytes(key, value);
	list.insert(key, value);
	return size;
}

template <class List>
void test_DualList(const char *name, List &&list1, List &&list2){
	mytest.begin(name);

	size_t bytes =
		// to have older timestamp
		listInsert(list2, "3 city",	"DIRTY"	);

	sleep();

	bytes +=
		listInsert(list1, "1 name",	"Niki"	) +
		listInsert(list1, "3 city",	"Sofia"	) +
		listInsert(list2, "2 age",	"22"	) +
		listInsert(list2, "4 os",	"Linux"	) +
	0;

	using MyMultiList = hm4::multi::DualList<const List, const List, hm4::multi::DualListEraseType::NORMAL>;

	MyMultiList list{ list1, list2 };

	return list_test(list, 4, bytes);
}

template <class List>
void test_DualListEmpty(const char *name, List &&list1, List &&list2){
	size_t bytes =
		listInsert(list2, "1 name",	"Niki"	) +
		listInsert(list2, "2 age",	"22"	) +
		listInsert(list2, "3 city",	"Sofia"	) +
		listInsert(list2, "4 os",	"Linux"	) +
	0;

	using MyMultiList = hm4::multi::DualList<List const, List const, hm4::multi::DualListEraseType::NORMAL>;

	mytest.begin(name);

	MyMultiList list01{ list1, list2 };

	list_test(list01, 4, bytes);

	mytest.begin(name);

	MyMultiList list10{ list1, list2 };

	list_test(list10, 4, bytes);
}

#include "multi/collectionlist.h"

template <class List>
void test_CollectionList(const char *name){
	using Vector = std::vector<List>;
	Vector v;

	auto f = [](){
		return List{ allocator };
	};

	size_t bytes = 0;

	{
		List l = f();
		l.insert("1 name",		"DIRTY"	);
		l.insert("2 age",		"DIRTY"	);

		bytes += l.bytes();

		v.push_back(f());
		v.push_back(std::move(l));
		v.push_back(f());
	}

	sleep();

	{
		List l = f();
		l.insert("1 name",		"Niki"	);
		l.insert("2 age",		"DIRTY"	);

		bytes += l.bytes();

		v.push_back(f());
		v.push_back(std::move(l));
		v.push_back(f());
	}

	sleep();

	{
		List l = f();
		l.insert("2 age",		"22"	);
		l.insert("3 city",		"Sofia"	);
		l.insert("4 os",		"Linux"	);

		bytes += l.bytes();

		v.push_back(f());
		v.push_back(std::move(l));
		v.push_back(f());
	}

	{
		mytest.begin(name);

		using MyMultiTable = hm4::multi::CollectionList<Vector>;

		MyMultiTable table{ v };

		list_test(table, 4, bytes);
	}
}

// ==============================

#include "vectorlist.h"

int main(){
	using List = hm4::VectorList<Allocator>;

	test_DualListEmpty	<List>("DualList (Empty)"	, List{ allocator }, List{ allocator }	);
	test_DualList		<List>("DualList"		, List{ allocator }, List{ allocator }	);
	test_CollectionList	<List>("CollectionList"							);

	return mytest.end();
}


