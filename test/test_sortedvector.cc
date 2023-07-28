#include "sortedvector.h"

#include "mytest.h"

#include <iostream>

MyTest mytest;

template<typename T>
void test_sortedvector(){
	auto print = [](auto const &v){
		for(auto &x : v)
			std::cout << x << ", ";

		std::cout << "\n";
	};

	mytest.begin("SortedVector");

	SortedVector<T,11> v{ { 1,2,3,4,5,7 } };

	print(v);

	mytest("size",		v.size() == 6			);
	mytest("empty",		!v.empty()			);
	mytest("capacity",	v.capacity() == 11		);
	mytest("front",		v.front() == 1			);
	mytest("back",		v.back() == 7			);

	mytest("search",	 v.search(T{6}, std::true_type{}	) == std::end(v)	);
	mytest("search",	*v.search(T{6}, std::false_type{}	) == 7			);
	mytest("search",	*v.search(T{7}, std::true_type{}	) == 7			);

	v.erase(100);
	v.erase(2);

	mytest("erase size",	v.size()  == 5			);

	v.erase(100);
	v.erase(7);

	mytest("erase size",	v.size()  == 4			);

	v.insert(4); // replace
	v.insert(5); // replace

	mytest("replace size",	v.size()  == 4			);

	v.insert(2);
	v.insert(0);
	v.insert(6);
	v.insert(7);

	mytest("insert size",	v.size()  == 8			);

	{
		constexpr SortedVector<T, 3> v2a{ { 10, 11, 12 } };
		v.merge(v2a);

		constexpr SortedVector<T, 100> v2b;
		v.merge(v2b);
	}

	print(v);

	mytest("merge size",	v.size()  == 11			);
	mytest("merge size",	v.size()  == v.capacity()	);

	{
		SortedVector<T, 6> v3;

		v.split(v3);

		print(v);
		print(v3);
	}

	mytest("split size",	v.size()  == 6			);

	v.destruct();
}

template<typename T>
void test_sortedvector_res(){
	auto print = [](auto const &v){
		for(auto &x : v)
			std::cout << *x << ", ";

		std::cout << "\n";
	};

	auto mk_construct = [](T val){
		return [val](auto &ptr){
			ptr = (T *) malloc(sizeof(T));
			*ptr = val;
			return true;
		};
	};

	auto destruct = [](auto &ptr){
		free(ptr);
	};

	auto comp = [](const T *a, T const &b){
		return comparator::comp(*a, b);
	};

	mytest.begin("SortedVector with memory management");

	SortedVector<T*, 100> v;

	mytest("size",		v.size() == 0			);
	mytest("empty",		v.empty()			);

	v.insert(T{1}, comp, mk_construct(1), destruct);
	v.insert(T{2}, comp, mk_construct(2), destruct);
	v.insert(T{3}, comp, mk_construct(3), destruct);
	v.insert(T{4}, comp, mk_construct(4), destruct);
	v.insert(T{7}, comp, mk_construct(7), destruct);
	v.insert(T{8}, comp, mk_construct(8), destruct);

	v.erase(T{8}, comp, destruct);

	print(v);

	mytest("size",		v.size() == 5			);
	mytest("empty",		!v.empty()			);
	mytest("capacity",	v.capacity() == 100		);
	mytest("front",		*v.front() == 1			);
	mytest("back",		*v.back()  == 7			);

	mytest("search",	  v.search(T{6}, comp, std::true_type{}		) == std::end(v)	);
	mytest("search",	**v.search(T{6}, comp, std::false_type{}	) == 7			);
	mytest("search",	**v.search(T{7}, comp, std::true_type{}		) == 7			);

	v.erase(T{0}, comp, destruct);
	v.erase(T{2}, comp, destruct);

	mytest("erase size",	v.size() == 4			);

	v.erase(T{0}, comp, destruct);
	v.erase(T{7}, comp, destruct);

	mytest("erase size",	v.size() == 3			);

	v.insert(T{3}, comp, mk_construct(3), destruct);	// replace
	v.insert(T{4}, comp, mk_construct(4), destruct);	// replace

	mytest("replace size",	v.size() == 3			);

	print(v);

	{
		SortedVector<T*, 100> v2a;

		v2a.insert(T{10}, comp, mk_construct(10), destruct);
		v2a.insert(T{20}, comp, mk_construct(20), destruct);
		v2a.insert(T{30}, comp, mk_construct(30), destruct);
		v2a.insert(T{40}, comp, mk_construct(40), destruct);
		v2a.insert(T{50}, comp, mk_construct(50), destruct);

		v.merge(v2a);

		constexpr SortedVector<T*, 100> v2b;
		v.merge(v2b);

		mytest("merge size",	v.size() == 8			);
	}

	print(v);

	{
		SortedVector<T*, 6> v3;

		v.split(v3);

		print(v);
		print(v3);

		mytest("split size",	v.size()  == 4			);
		mytest("split size",	v3.size() == 4			);

		v3.destruct(destruct);
	}

	v.destruct(destruct);
}

#include <cstdint>

int main(){
	test_sortedvector<int>();

	test_sortedvector<int16_t>();
	test_sortedvector<int64_t>();

	test_sortedvector<uint16_t>();
	test_sortedvector<uint64_t>();

	// ------

	test_sortedvector_res<int>();

	test_sortedvector_res<int16_t>();
	test_sortedvector_res<int64_t>();

	test_sortedvector_res<uint16_t>();
	test_sortedvector_res<uint64_t>();

	return mytest.end();
}

