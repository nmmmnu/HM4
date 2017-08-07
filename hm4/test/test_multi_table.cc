#include "pair.h"

using Pair	= hm4::Pair;

#include "mytest.h"

MyTest mytest;


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
void table_test(const LIST &list, typename LIST::size_type const size, size_t const bytes){
	// GENERAL

	mytest("count",			list.size() == size		);
	mytest("count estim",		list.size(true) >= size		);
	mytest("empty",			! list.empty()			);
	mytest("sizeof",		list.bytes() == bytes		);


	Pair p;


	// GET

	p = list["3 city"];
	mytest("get",			p.getVal() == "Sofia"		);

	p = list["nonexistent"];
	mytest("get non existent",	! p				);


	// ITERATOR

	iterator_test(list);
	iterator_get_test(list);


	// MOVE

	const LIST mlist = std::move(list);
	mytest("move c-tor 1",		mlist.bytes() == bytes		);
	mytest("move c-tor 2",		! list.empty()			);
}


#include "multi/duallist.h"

template <class LIST>
size_t list_insert(LIST &list, Pair &&p){
	const size_t size = p.bytes();
	list.insert(std::move(p));
	return size;
}

template <class LIST>
void test_DualList(const char *name){
	mytest.begin(name);

	LIST list1;
	LIST list2;

	size_t bytes = 0;

	// to have older timestamp
	bytes += list_insert(list2, {"3 city",	"DIRTY"	});

	bytes += list_insert(list1, {"1 name",	"Niki"	});
	bytes += list_insert(list1, {"3 city",	"Sofia"	});

	bytes += list_insert(list2, {"2 age",	"22"	});
	bytes += list_insert(list2, {"4 os",	"Linux"	});

	using MyMultiTable = hm4::multi::DualList<const LIST, const LIST>;

	MyMultiTable table{ list1, list2 };

	return table_test(table, 4, bytes);
}

template <class LIST>
void test_DualListEmpty(const char *name){
	mytest.begin(name);

	LIST list1;
	LIST list2;

	size_t bytes = 0;

	bytes += list_insert(list2, {"1 name",	"Niki"	});
	bytes += list_insert(list2, {"2 age",	"22"	});
	bytes += list_insert(list2, {"3 city",	"Sofia"	});
	bytes += list_insert(list2, {"4 os",	"Linux"	});

	using MyMultiTable = hm4::multi::DualList<const LIST, const LIST>;

	MyMultiTable table{ list1, list2 };

	return table_test(table, 4, bytes);
}

#include "multi/collectionlist.h"

template <class LIST>
void test_CollectionList(const char *name){
	mytest.begin(name);

	using MyContainer = std::vector<LIST>;
	MyContainer container;

	size_t bytes = 0;

	{
		LIST l;
		l.insert( {"1 name",		"DIRTY"	} );
		l.insert( {"2 age",		"DIRTY"	} );

		bytes += l.bytes();

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}

	{
		LIST l;
		l.insert( {"1 name",		"Niki"	} );
		l.insert( {"2 age",		"DIRTY"	} );

		bytes += l.bytes();

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}

	{
		LIST l;
		l.insert( {"2 age",		"22"	} );
		l.insert( {"3 city",		"Sofia"	} );
		l.insert( {"4 os",		"Linux"	} );

		bytes += l.bytes();

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}

	using MyMultiTable = hm4::multi::CollectionList<MyContainer>;

	MyMultiTable table{ container };

	return table_test(table, 4, bytes);
}


#include "vectorlist.h"

using List = hm4::VectorList;

int main(int argc, char **argv){
	test_DualList<hm4::VectorList		>("DualList"		);
	test_DualListEmpty<hm4::VectorList	>("DualList (Empty)"	);
	test_CollectionList<hm4::VectorList	>("CollectionList"	);

	return mytest.end();
}


