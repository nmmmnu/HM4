#ifndef MY_SPARSE_ARRAY_MYPAIR_
#define MY_SPARSE_ARRAY_MYPAIR_

namespace mysparsearray{

	template<typename T1, typename T2>
	struct MyPair{
		T1 first	{};
		T2 second	{};

		constexpr MyPair() = default;

		template<typename UT1>
		constexpr MyPair(UT1 &&first) :
					first	(std::forward<UT1>(first	)){}

		template<typename UT1, typename UT2>
		constexpr MyPair(UT1 &&first, UT2 &&second) :
					first	(std::forward<UT1>(first	)),
					second	(std::forward<UT2>(second	)){}
	};

} // namespace mysparsemap

#endif

