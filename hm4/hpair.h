#ifndef CPAIR_H
#define CPAIR_H

#include "pair.h"

#include "stringhash.h"

//#include <type_traits>

namespace hm4{

	namespace HPair{
		using HKey		= uint64_t;
		using SS		= StringHash<HKey>;

		constexpr auto N	= SS::N;

		inline int cmp(HKey const src_hkey, Pair const &src_pair, HKey const hkey, std::string_view const key){
			auto [ ok, result ] = SS::compare(src_hkey, hkey);

			// if OK is true,  this means, key.size() <  HPair::N  &&  src_pair.getKey().size() <  HPair::N,
			// if OK is false, this means, key.size() >= HPair::N  &&  src_pair.getKey().size() <= HPair::N,
			// so we can skip comparing first HPair::N characters

			return ok ? result : src_pair.cmpX<N>(key);
		}

		template<bool B>
		inline auto toStringView(HKey const &hkey, std::bool_constant<B>){
			// must be const ref to preserve the address...
			const char *s = reinterpret_cast<const char *>(& hkey);

			if constexpr(B)
				return std::string_view{ s, strnlen(s, N) };
			else
				return std::string_view{ s, N };
		}

	} // namespace

} // namespace hm4

#endif

