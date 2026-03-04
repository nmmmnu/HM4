#ifndef MY_HASHTABLE_MYPAIR_H_
#define MY_HASHTABLE_MYPAIR_H_

namespace myhashtable{

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

		[[nodiscard]]
		constexpr bool operator ==(MyPair const &other) const{
			return first == other.first && second == other.second;
		}
	};

} //namespace myhashtable

#endif

