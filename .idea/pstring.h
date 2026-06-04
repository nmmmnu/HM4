#ifndef P_STRING_H_
#define P_STRING_H_

#include "mystring.h"
#include "myendian.h"

namespace pstring{

	template<typename T>
	constexpr void clear(T &x){
		x.csize = 0;
	}

	template<typename T>
	constexpr size_t size(T const &x){
		return betoh(x.csize);
	}

	template<typename T>
	constexpr bool empty(T const &x){
		return !x.csize;
	}

	template<typename T1, typename T2>
	constexpr int cmp(T1 const &x1, T2 const &x2){
		return compare(x1.cdata, size(x1), x2.cdata, size(x2));
	}

	template<typename T1, typename T2>
	constexpr int eq(T1 const &x1, T2 const &x2){
		return equals(x1.cdata, size(x1), x2.cdata, size(x2));
	}

	template<typename T>
	constexpr std::string_view get(T const &x){
		return std::string_view{ x.cdata, size(x) };
	}

	template<typename T>
	constexpr bool set(T &x, std::string_view sv){
		if (sv.size() > x.ccapacity)
			return false;

		using size_type = typename T::size_type;

		x.csize = htobe(
			static_cast<size_type>(sv.size())
		);

		memcpy(x.cdata, sv.data(), sv.size());

		return true;
	}

	template<typename T>
	constexpr std::string_view getClear(T &x){
		auto sv = get(x);
		clear(x);
		return sv;
	}

} // namespace pstring

#endif

