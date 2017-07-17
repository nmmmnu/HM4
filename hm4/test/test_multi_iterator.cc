#include "pair.h"

using Pair	= hm4::Pair;



#include "mytest.h"

MyTest mytest;



template<class IT, class IT2>
void iterator_test(const char *title, IT it1, const IT &end1, IT2 it2){
	mytest.begin(title);

	while(it1 != end1){
		const auto &p1 = *it1;
		const auto &p2 = *it2;

	//	p1.print();
	//	p2.print();

		bool const equal =
				p1.getKey() == p2.getKey() &&
				p1.getVal() == p2.getVal();

		if (! equal){
			mytest("Elements are different", false);
		}

		++it1;
		++it2;
	}
}



template<class LIST>
void test_normal_iterator(){
	LIST list;
	list.insert( {"1 name",		"Niki"	} );
	list.insert( {"2 age",		"22"	} );
	list.insert( {"3 city",		"Sofia"	} );
	list.insert( {"4 os",		"Linux"	} );

	LIST clist;
	clist.insert( {"1 name",	"Niki"	} );
	clist.insert( {"2 age",		"22"	} );
	clist.insert( {"3 city",	"Sofia"	} );
	clist.insert( {"4 os",		"Linux"	} );

	iterator_test(
			"Normal Iterator",

			list.begin(),
			list.end(),

			clist.begin()
	);

	iterator_test(
			"Normal Iterator Lower Bound",

			list.lowerBound("2 age"),
			list.end(),

			clist.lowerBound("2 age")
	);

}

#include "multiiterator/dualiterator.h"

template<class LIST>
void test_dual_iterator(){
	LIST list1;
	list1.insert( {"1 name",	"DIRTY"	} );
	list1.insert( {"3 city",	"Sofia"	} );

	LIST list2;
	list1.insert( {"1 name",	"Niki"	} );
	list2.insert( {"2 age",		"22"	} );
	list2.insert( {"4 os",		"Linux"	} );

	LIST clist;
	clist.insert( {"1 name",	"Niki"	} );
	clist.insert( {"2 age",		"22"	} );
	clist.insert( {"3 city",	"Sofia"	} );
	clist.insert( {"4 os",		"Linux"	} );

	using Iterator = hm4::multiiterator::DualIterator<LIST, LIST>;

	iterator_test(
			"Dual Iterator",

			Iterator(list1, list2, false),
			Iterator(list1, list2, true),

			clist.begin()
	);

	iterator_test(
			"Dual Iterator Lower Bound",

			Iterator(list1, list2, (const char *) "2 age"),
			Iterator(list1, list2, true),

			clist.lowerBound("2 age")
	);

	LIST elist;

	iterator_test(
			"Dual Iterator Empty List",

			Iterator(elist, clist, false),
			Iterator(elist, clist, true),

			clist.begin()
	);

}

#include "multiiterator/collectioniterator.h"

template<class LIST>
void test_collection_iterator(){
	std::vector<LIST> container;

	{
		LIST l;
		l.insert( {"1 name",		"DIRTY"	} );
		l.insert( {"2 age",		"DIRTY"	} );

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}

	{
		LIST l;
		l.insert( {"1 name",		"Niki"	} );
		l.insert( {"2 age",		"DIRTY"	} );

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}

	{
		LIST l;
		l.insert( {"2 age",		"22"	} );
		l.insert( {"3 city",		"Sofia"	} );
		l.insert( {"4 os",		"Linux"	} );

		container.push_back(LIST{});
		container.push_back(std::move(l));
		container.push_back(LIST{});
	}


	LIST clist;
	clist.insert( {"1 name",	"Niki"	} );
	clist.insert( {"2 age",		"22"	} );
	clist.insert( {"3 city",	"Sofia"	} );
	clist.insert( {"4 os",		"Linux"	} );


	using Iterator = hm4::multiiterator::CollectionIterator<LIST>;

	iterator_test(
			"Collection Iterator",

			Iterator(container, false),
			Iterator(container, true),

			clist.begin()
	);

	iterator_test(
			"Collection Iterator Lower Bound",

			Iterator(container, (const char *) "2 age"),
			Iterator(container, true),

			clist.lowerBound("2 age")
	);
}

#include "vectorlist.h"

int main(int argc, char **argv){
	using List = hm4::VectorList;	// any list will do

	test_normal_iterator<List>();
	test_dual_iterator<List>();
	test_collection_iterator<List>();

	return mytest.end();
}



