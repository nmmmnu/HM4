#include "complexbinarysearch.h"
#include "comparator.h"
#include "my_typename.h"

#include <iostream>
#include <cassert>

template<class S>
class STest{
private:
	using T  = typename S::value_type;
	using D  = typename S::difference_type;
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
		[[maybe_unused]]
		auto comp2 = [&](auto key, D, D &mid, D){
			return comparator::comp(s.v[mid], key);
		};

		[[maybe_unused]]
		auto comp = [&](auto key, auto start, auto &mid, auto end){
			size_t const max_count = 3;

			auto p = [&](auto label, auto mid){
				std::cout << label << " : " << mid << " = " << (long long int) s.v[mid] << '\n';
			};

			p('N', mid);
			int const cmp = comparator::comp(s.v[mid], key);

			if (cmp < 0){
				// go right
				size_t count = 0;
				while(mid + 1 < end && count++ < max_count){
					int const cmp2 = comparator::comp(s.v[mid + 1], key);

					if (cmp2 > 0)
						break;

					++mid;

					p('L', mid);

					if (cmp2 == 0)
						return 0;
				}
			}else if (cmp > 0){
				// go left
				size_t count = 0;
				while(mid > start && count++ < max_count){
					int const cmp2 = comparator::comp(s.v[mid - 1], key);

					if (cmp2 < 0)
						break;

					--mid;

					p('R', mid);

					if (cmp2 == 0)
						return 0;
				}
			}

			return cmp;
		};

		const auto p = complexBinarySearch(s.MAX, key, comp);

		if (p.pos == s.MAX){
			std::cout
				<<	s.name << '<' << my_typename<T> << '>'	<< '\t'
				<<	b(p.found)			<< '\t'
				<<	b(found)			<< '\t'
				<<	(int) key			<< '\t'
				<<	'-'				<< '\t'
				<<	'-'				<< '\t'
				<<	"OK!"				<< '\n'
			;

			return p.found == found && found == false;
		}else{
			auto const &value = s.v[p.pos];

			std::cout
				<<	s.name << '<' << my_typename<T> << '>'	<< '\t'
				<<	b(p.found)			<< '\t'
				<<	b(found)			<< '\t'
				<<	(int) key			<< '\t'
				<<	(int) value			<< '\t'
				<<	(int) result			<< '\t'
				<<	"OK!"				<< '\n'
			;

			assert(p.found == found);
			assert(value == result);

			return p.found == found && value == result;
		}
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
	using difference_type = size_t;

	constexpr static const char *name = "array";

	constexpr static difference_type MAX  = 20;

	T v[MAX] = {
			10, 11, 12, 13, 14, /* 15, */
			16, 17, 18, 19, 20,
			21, 22, 23, 24, 25,
			26, 27, 28, 29, 30
	};
};

template<typename T>
void test_int(){
	STest<S1<T> >()();
}

#include <cstdint>

int main(){
	/*
	test_int<int8_t		>();
	test_int<int16_t	>();
	test_int<int32_t	>();
	test_int<int64_t	>();

	test_int<uint8_t	>();
	test_int<uint16_t	>();
	test_int<uint32_t	>();
	* */
	test_int<uint64_t	>();
}

