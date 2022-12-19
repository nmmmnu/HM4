#ifndef THREE_WAY_QUICKSORT_H_
#define THREE_WAY_QUICKSORT_H_

#include <string_view>
#include <cstdio>	// also provides size_t
#include <functional>	// std::invoke

namespace three_way_quicksort_implementation_{
	namespace{

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

		constexpr static auto subAt(std::string_view s, size_t digit){
			return digit < s.size() ? s.substr(digit) : "";
		}

		template<typename It, typename Projection>
		bool compareLT(It a, It b, size_t digit, Projection &p){
			return subAt(std::invoke(p, *a), digit) < subAt(std::invoke(p, *b), digit);
		}

		template<typename It, typename Projection>
		bool compareGT(It a, It b, size_t digit, Projection &p){
			return compareLT(b, a, digit, p);
		}

	} // namespace



	template<typename It, typename Projection>
	void super2_Sort(It it, size_t digit, Projection p){
		auto &a = it;
		auto b = std::next(a);

		auto _ = [digit, &p](auto a, auto b){
			if (compareGT(a, b, digit, p))
				std::iter_swap(b, a);
		};

		_(a, b);

	}



	template<typename It, typename Projection>
	void super3_Sort(It it, size_t digit, Projection p){
		auto &a = it;
		auto b = std::next(a);
		auto c = std::next(b);

		auto _ = [digit, &p](auto a, auto b){
			if (compareGT(a, b, digit, p))
				std::iter_swap(b, a);
		};

		_(a, c);
		_(a, b);
		_(b, c);
	}



	template<typename It, typename Projection>
	void insertion_Sort(It first, It last, size_t digit, Projection p){
		for (It i = first; i < last; ++i)
			for (It j = i; j > first && compareLT(j, j - 1, digit, p); --j)
				std::iter_swap(j, j - 1);
	}



	template<typename It, typename Projection>
	void shell_Sort(It first, It last, size_t digit, Projection p){
		size_t const size = std::distance(first, last);

		if (size <= 1)
			return;

		size_t step = size - 1;

		for(;;){
			size_t ix_first = 0;
			size_t ix_last  = size;

			while(ix_first < ix_last){
				if constexpr(true){
					bool sorted = true;
					size_t ix = ix_first;

					for (size_t i = ix_first; i < ix_last - step; ++i){
						auto const j = i + step;

						if (compareGT(first + i, first + j, digit, p)){
							std::iter_swap(first + i, first + j);

							sorted = false;
							ix = i;
						}
					}

					if (sorted)
						break;

					ix_last = ix + 1;
				}

				if constexpr(true){
					bool sorted = true;
					size_t ix = ix_last;

					for (size_t i = ix_last; i --> ix_first + step;){
						auto const j = i - step;

						if (compareLT(first + i, first + j, digit, p)){
							std::iter_swap(first + i, first + j);

							sorted = false;
							ix = i;
						}
					}

					if (sorted)
						break;

					ix_first = ix;
				}
			}

			if (step == 1)
				break;

			step = step >> 1;

			if (step < 1)
				step = 1;
		}
	}



	// Based on:
	// https://stackoverflow.com/questions/6972635/efficient-string-sorting-algorithm
	// https://www.geeksforgeeks.org/3-way-radix-quicksort-in-java/

	template<typename Projection>
	struct Quick3Way_Sort{
		constexpr static size_t MIN_CUTOFF_DEEP = 16;

		Quick3Way_Sort(Projection p, size_t cuttof_deep = MIN_CUTOFF_DEEP) :
					p		(std::move(p)							),
					cuttof_deep	(cuttof_deep < MIN_CUTOFF_DEEP ? MIN_CUTOFF_DEEP : cuttof_deep	){}

		template<typename It>
		void operator()(It first, It last, size_t digit) const{
			return sort(first, last, digit, 0);
		}

	private:
		template<typename It>
		void median(It first, It last, size_t digit) const{
			It mid = first + ((last - first) >> 1);

			auto _ = [this, digit](auto x){
				return charAt(x, digit);
			};

			if (_(mid) < _(first))
				std::iter_swap(mid, first);

			if (_(mid) < _(last))
				std::iter_swap(mid, last);

			if (_(first) < _(last))
				std::iter_swap(first, last);
		}

		template<typename It>
		void sort(It first, It last, size_t digit, size_t deep) const{
			++deep;

			// controls tail recursion.
			for(;;){
				auto const distance = std::distance(first, last);

				switch(distance){
				case 0:
				case 1:  return;
				case 2:  return super2_Sort(first, digit, p);
				case 3:  return super3_Sort(first, digit, p);
				case 4:
				case 5:
				case 6:  return insertion_Sort(first, last, digit, p);
				default: break;
				}

				if (deep > cuttof_deep){
				//	fprintf(stderr, "Cut Off: too deep recursion %zu\n", distance);

					// if we are here, the distance is great than 4
					return shell_Sort(first, last, digit, p);
				}

				auto lt = first;
				auto gt = last  - 1;
				auto it = first + 1;

				median(lt, gt, digit);

				// partition

				auto const pivot = charAt(lt, digit);

				while (it <= gt) {
					auto const t = charAt(it, digit);

					if (t < pivot)
						std::iter_swap(lt++, it++);
					else if (t > pivot)
						std::iter_swap(it, gt--);
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
		int charAt(It it, size_t digit) const{
			return charAt(std::invoke(p, *it), digit);
		}

	private:
		Projection	p;
		size_t		cuttof_deep;
	};



	template<typename It, typename Projection>
	void doInsertionSort(It lo, It hi, Projection p){
		size_t const digit = 0;
		return insertion_Sort(lo, hi, digit, std::move(p));
	}

	template<typename It, typename Projection>
	void doShellSort(It lo, It hi, Projection p){
		size_t const digit = 0;
		return shell_Sort(lo, hi, digit, std::move(p));
	}

	template<typename It, typename Projection>
	void doThreeWayQuickSort(It lo, It hi, Projection p){
		Quick3Way_Sort<Projection> sort{ std::move(p) };

		size_t const digit = 0;
		return sort(lo, hi, digit);
	}
}

template<typename It, typename Projection>
void threeWayQuickSort(It first, It last, Projection p){
	using namespace three_way_quicksort_implementation_;

	return doThreeWayQuickSort(first, last, std::move(p));
}

template<typename It>
void threeWayQuickSort(It first, It last){
	using namespace three_way_quicksort_implementation_;

	auto p = [](auto const &a) -> auto const &{
		return a;
	};

	return threeWayQuickSort(first, last, p);
}

#endif

