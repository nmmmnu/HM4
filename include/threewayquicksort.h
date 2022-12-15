#ifndef THREE_WAY_QUICKSORT_H_
#define THREE_WAY_QUICKSORT_H_

#include <string_view>
#include <cstdio>	// also provides size_t
#include <functional>	// std::invoke

namespace three_way_quicksort_implementation_{
	namespace{

		template<typename It>
		void swapIt(It a, It b){
			using std::swap;
			swap(*a, *b);
		}



		enum class M3{
			a,
			b,
			c
		};

		template<typename T>
		constexpr M3 max3(T const &a, T const &b, T const &c){
			if (a > b){
				if (a > c)
					return M3::a;
				else
					return M3::c;
			}else{
				if (b > c)
					return M3::b;
				else
					return M3::c;
			}
		}

	} // namespace



	template<typename Projection>
	struct Insertion_Sort{
		Insertion_Sort(Projection p) : p(std::move(p)){}

		template<typename It>
		void operator()(It first, It last, size_t digit) const{
			for (It i = first; i < last; ++i)
				for (It j = i; j > first && compare(j, j - 1, digit); --j)
					swapIt(j, j - 1);
		}

	private:
		constexpr static auto subAt(std::string_view s, size_t digit){
			return digit < s.size() ? s.substr(digit) : "";
		}

		template<typename It>
		bool compare(It a, It b, size_t digit) const{
			return subAt(std::invoke(p, *a), digit) < subAt(std::invoke(p, *b), digit);
		}

	private:
		Projection p;
	};



	template<typename Projection>
	struct Shell_Sort{
		Shell_Sort(Projection p) : p(std::move(p)){}

		template<typename It>
		void operator()(It first, It last, size_t digit) const{
			size_t const size = std::distance(first, last);

			if (size <= 1)
				return;

			size_t step = size - 1;

			for(;;){
				bool sorted = false;

				while ( ! sorted ){
					sorted = true;

					for (size_t i = 0; i < size - step; ++i){
						auto j = i + step;

						if (compare(first + i, first + j, digit)){
							swapIt(first + i, first + j);

							sorted = false;
						}
					}
				}

				if (sorted && step == 1)
					break;

				step = step / 2;

				if (step < 1)
					step = 1;
			}
		}

	private:
		constexpr static auto subAt(std::string_view s, size_t digit){
			return digit < s.size() ? s.substr(digit) : "";
		}

		template<typename It>
		constexpr bool compare(It a, It b, size_t digit) const{
			return subAt(std::invoke(p, *a), digit) > subAt(std::invoke(p, *b), digit);
		}

	private:
		Projection p;
	};



	// Based on:
	// https://stackoverflow.com/questions/6972635/efficient-string-sorting-algorithm
	// https://www.geeksforgeeks.org/3-way-radix-quicksort-in-java/

	template<typename Projection>
	struct Quick3Way_Sort{
		constexpr static bool  CUTOFF_INS_ENABLED = false;
		constexpr static short CUTOFF_INS  = 4;
		constexpr static short CUTOFF_DEEP = 16;

		Quick3Way_Sort(Projection p) : p(std::move(p)){}

		template<typename It>
		void operator()(It first, It last, size_t digit, size_t deep) const{
			return sort(first, last, digit, deep);
		}

	private:
		template<typename It>
		void median(It first, It last, size_t digit) const{
			It mid = first + ((last - first) >> 1);

			if (charAt(mid, digit) < charAt(first, digit))
				swapIt(first, mid);

			if (charAt(mid, digit) < charAt(last, digit))
				swapIt(last, mid);

			if (charAt(first, digit) < charAt(last, digit))
				swapIt(first, last);
		}

		template<typename It>
		void sort(It first, It last, size_t digit, size_t deep) const{
			++deep;

			// controls tail recursion.
			for(;;){
				auto const distance = std::distance(first, last);

				if (distance <= 1)
					return;

				if constexpr(CUTOFF_INS_ENABLED){
					if (distance <= CUTOFF_INS){
					//	printf("Cut Off: distance %zu\n", std::distance(first, last) );

						Insertion_Sort<Projection> sort{p};

						return sort(first, last, digit);
					}
				}

				if (deep > CUTOFF_DEEP){
					size_t const size = std::distance(first, last);

					printf("Cut Off: too deep recursion %zu\n", size);

					if (size <= CUTOFF_INS){
						Insertion_Sort<Projection> sort{p};

						return sort(first, last, digit);
					}else{
						Shell_Sort<Projection> sort{p};

						return sort(first, last, digit);
					}
				}

				auto lt = first;
				auto gt = last - 1;
				auto it = first + 1;

				median(lt, gt, digit);

				// partition

				auto const pivot = charAt(lt, digit);

				while (it <= gt) {
					auto const t = charAt(it, digit);

					if (t < pivot)
						swapIt(lt++, it++);
					else if (t > pivot)
						swapIt(it, gt--);
					else
						++it;
				}

				// handle tail recursion

				auto const max = max3(
							std::distance(first, lt),
							std::distance(lt, gt + 1),
							std::distance(gt + 1, last)
				);

				switch(max){
				case M3::a:
					if (pivot >= 0)
						sort(lt, gt + 1, digit + 1, deep);

					sort(gt + 1, last, digit, deep);

					// sort(first, lt, digit, deep);

					// prepare tail recursion

					last = lt;

					break;

				case M3::b:
					sort(first, lt, digit, deep);

					sort(gt + 1, last, digit, deep);

					if (pivot >= 0){
						// sort(lt, gt + 1, digit + 1);

						// prepare tail recursion

						first = lt;
						last  = gt + 1;
						++digit;

						break;
					}

					return;

				case M3::c:
					sort(first, lt, digit, deep);

					if (pivot >= 0)
						sort(lt, gt + 1, digit + 1, deep);

					// sort(gt + 1, last, digit, deep);

					// prepare tail recursion

					first = gt + 1;

					break;
				}
			}
		}

	private:
		constexpr static int charAt(std::string_view s, size_t digit){
			return digit < s.size() ? s[digit] : -1;
		}

		template<typename It>
		constexpr int charAt(It it, size_t digit) const{
			return charAt(std::invoke(p, *it), digit);
		}

	private:
		Projection p;
	};



	template<typename It, typename Projection>
	void doInsertionSort(It lo, It hi, Projection p) {
		Insertion_Sort<Projection> sort{ std::move(p) };

		size_t const digit = 0;

		return sort(lo, hi, digit);
	}

	template<typename It, typename Projection>
	void doShellSort(It lo, It hi, Projection p) {
		Shell_Sort<Projection> sort{ std::move(p) };

		size_t const digit = 0;

		return sort(lo, hi, digit);
	}

	template<typename It, typename Projection>
	void doThreeWayQuickSort(It lo, It hi, Projection p) {
		Quick3Way_Sort<Projection> sort{ std::move(p) };

		size_t const digit = 0;
		size_t const deep  = 0;

		return sort(lo, hi, digit, deep);

	}



	auto standardProjection = [](auto const &a) -> auto const &{
		return a;
	};
}

template<typename It, typename Projection>
void threeWayQuickSort(It first, It last, Projection p){
	using namespace three_way_quicksort_implementation_;

	return doThreeWayQuickSort(first, last, std::move(p));
}

template<typename It>
void threeWayQuickSort(It first, It last){
	using namespace three_way_quicksort_implementation_;

	return doThreeWayQuickSort(first, last, standardProjection);
}

#endif

