#include "binarysearch.h"

#include "mytest.h"

MyTest mytest;


template<class T, size_t SIZE>
class VVector{
public:
	size_t size() const{
		return SIZE;
	}

	T operator [](size_t const index) const{
		return T(index);
	}
};

constexpr size_t MAX = 10000;

template<class T>
void test(const char *name, const T key){
	VVector<T, MAX> v;

	const auto p = binarySearch(v, size_t{ 0 }, v.size(), key, BinarySearchCompStdandard{});

	bool   const found	= p.first;
	size_t const result	= p.second;

	mytest(name, found && v[result] == key	);
}

template<class T>
void test(const char *name){
	test(name, T{   0 }	);
	test(name, T{ 100 }	);
	test(name, (T) MAX - 1	);
}

int main(){
	test<signed char	>("char"	);
	test<short		>("short"	);
	test<int		>("int"		);
	test<long		>("long"	);

	test<unsigned char	>("u char"	);
	test<unsigned short	>("u short"	);
	test<unsigned int	>("u int"	);
	test<unsigned long	>("u long"	);

	test<size_t		>("size_t"	);

	return mytest.end();
}

