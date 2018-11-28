#include "binarysearch.h"

#include "my_typename.h"

#include "stringref.h"

#include <iostream>
#include <cassert>

template<class S>
class STest{
private:
	using T  = typename S::value_type;
	S s;

public:
	void operator()() const{
		test(	 1, false, 10	);
		test(	10		);
		test(	14		);
		test(	15, false, 16	);
		test(	30		);
		test(	31, false, -1	);
		test(	99, false, -1	);
	}

private:
	static char b(bool const b){
		return b ? 'Y' : 'N';
	}

	bool test_do(T const key, bool const found, T const result) const{
		auto const end_it = std::end(s.v);

		const auto p = binarySearch(std::begin(s.v), end_it, key);

		if (p.it == end_it){
			std::cout
				<<	s.name << '<' << my_typename<T> << '>'	<< '\t'
				<<	b(p.found)			<< '\t'
				<<	b(found)			<< '\t'
				<<	'-'				<< '\t'
				<<	'-'				<< '\t'
				<<	"OK!"				<< '\n'
			;
		}else{
			std::cout
				<<	s.name << '<' << my_typename<T> << '>'	<< '\t'
				<<	b(p.found)			<< '\t'
				<<	b(found)			<< '\t'
				<<	(int) *p.it			<< '\t'
				<<	(int) result			<< '\t'
				<<	"OK!"				<< '\n'
			;
		}


		if (p.it == end_it)
			return p.found == false;

		return p.found == found && *p.it == result;
	}

	void test(int const key, bool const found, int const result) const{
		bool const ok = test_do(T(key), found, T(result) );

		assert(ok);
	}

	void test(int const key) const{
		test(key, true, key);
	}
};


#include <forward_list>

template<class T>
struct S1{
	using value_type = T;

	const char *name = "rnd";

	constexpr static int MAX  = 20;

	T v[MAX] = {
			10, 11, 12, 13, 14, /* 15, */
			16, 17, 18, 19, 20,
			21, 22, 23, 24, 25,
			26, 27, 28, 29, 30
	};
};

template<class T>
struct S2{
	using value_type = T;

	const char *name = "inp";

	std::forward_list<T> v {
			10, 11, 12, 13, 14, /* 15, */
			16, 17, 18, 19, 20,
			21, 22, 23, 24, 25,
			26, 27, 28, 29, 30
	};
};

template<typename T>
void test_int(){
	STest<S1<T> >()();
	STest<S2<T> >()();
}

#include <cstdint>

int main(){
	test_int<int8_t		>();
	test_int<int16_t	>();
	test_int<int32_t	>();
	test_int<int64_t	>();

	test_int<uint8_t	>();
	test_int<uint16_t	>();
	test_int<uint32_t	>();
	test_int<uint64_t	>();
}

