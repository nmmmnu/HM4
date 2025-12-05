#ifndef CMP_LCP_H_
#define CMP_LCP_H_

#include <cstdint>
#include <cassert>
#include <string_view>

namespace cmp_lcp_impl_{

	constexpr int sgn(uint8_t a, uint8_t b){
		return (a > b) - (a < b);
	};

	constexpr int sgn(char a, char b){
		return sgn(
			static_cast<uint8_t>(a),
			static_cast<uint8_t>(b)
		);
	};



	struct ResultLCP{
		int     cmp;
		size_t	prefix;
	};



	constexpr size_t firstDifferentByte(uint64_t const a){
		#if	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
			auto const b = __builtin_clzll(a);
		#elif	defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
			auto const b = __builtin_ctzll(a);
		#else
			#error "I can handle only big and little endian"
		#endif

		return static_cast<size_t>(b >> 3);
	}

	constexpr ResultLCP cmpLong(std::string_view a, std::string_view b, size_t const size){
		size_t i = 0;

		while (i + 8 <= size){
			auto const va = *(const uint64_t *)(a.data() + i);
			auto const vb = *(const uint64_t *)(b.data() + i);

			if (auto const xorv = va ^ vb; xorv){
				size_t const off = firstDifferentByte(xorv);

				i += off;

				return { sgn(a[i], b[i]), i };
			}

			i += 8;
		}

		while (i < size){
			if (a[i] != b[i])
				return { sgn(a[i], b[i]), i };

			++i;
		}

		return { 0, i };
	}



	namespace comp_{

		struct Comp{
			std::string_view	a;
			std::string_view	b;
			bool			done	= false;
			size_t			i	= 0;
			int			result	= 0;

			constexpr operator bool() const{
				return done;
			}

			template<typename T>
			constexpr void block(){
				if (done)
					return;

				if (int const comp = comp_<T>(); comp != 0){
					result = comp;
					done = true;
				}else{
					i += sizeof(T);
				}
			}

		private:
			template<typename T>
			constexpr int comp_(){
				static_assert(
					std::is_same_v<T, uint16_t> ||
					std::is_same_v<T, uint32_t> ||
					std::is_same_v<T, uint64_t>
				);

				auto const va = *(const T *)(a.data() + i);
				auto const vb = *(const T *)(b.data() + i);

				if (va != vb){
					uint64_t const xorv = va ^ vb;

					i += firstDifferentByte(xorv);

					return sgn(a[i], b[i]);
				}

				return 0;
			}
		};

		template<>
		constexpr int Comp::comp_<uint8_t>(){
			return sgn(a[i], b[i]);
		}

	} // namespace comp



	constexpr ResultLCP cmpMain(std::string_view a, std::string_view b, size_t const size){
		if (size > 32)
			return cmpLong(a, b, size);

		auto c = comp_::Comp{ a, b };

		using _1 = uint8_t;
		using _2 = uint16_t;
		using _4 = uint32_t;
		using _8 = uint64_t;

		switch(size){
		case  0: 												return { c.result, c.i };
		case  1: c.block<_1>();											return { c.result, c.i };
		case  2: c.block<_2>();											return { c.result, c.i };
		case  3: c.block<_2>(); c.block<_1>();									return { c.result, c.i };
		case  4: c.block<_4>();											return { c.result, c.i };
		case  5: c.block<_4>(); c.block<_1>();									return { c.result, c.i };
		case  6: c.block<_4>(); c.block<_2>();									return { c.result, c.i };
		case  7: c.block<_4>(); c.block<_2>(); c.block<_1>();							return { c.result, c.i };
		case  8: c.block<_8>();											return { c.result, c.i };
		case  9: c.block<_8>(); c.block<_1>();									return { c.result, c.i };
		case 10: c.block<_8>(); c.block<_2>();									return { c.result, c.i };
		case 11: c.block<_8>(); c.block<_2>(); c.block<_1>();							return { c.result, c.i };
		case 12: c.block<_8>(); c.block<_4>();									return { c.result, c.i };
		case 13: c.block<_8>(); c.block<_4>(); c.block<_1>();							return { c.result, c.i };
		case 14: c.block<_8>(); c.block<_4>(); c.block<_2>();							return { c.result, c.i };
		case 15: c.block<_8>(); c.block<_4>(); c.block<_2>(); c.block<_1>();					return { c.result, c.i };
		case 16: c.block<_8>(); c.block<_8>();									return { c.result, c.i };
		case 17: c.block<_8>(); c.block<_8>(); c.block<_1>();							return { c.result, c.i };
		case 18: c.block<_8>(); c.block<_8>(); c.block<_2>();							return { c.result, c.i };
		case 19: c.block<_8>(); c.block<_8>(); c.block<_2>(); c.block<_1>();					return { c.result, c.i };
		case 20: c.block<_8>(); c.block<_8>(); c.block<_4>();							return { c.result, c.i };
		case 21: c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_1>();					return { c.result, c.i };
		case 22: c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_2>();					return { c.result, c.i };
		case 23: c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_2>(); c.block<_1>();			return { c.result, c.i };
		case 24: c.block<_8>(); c.block<_8>(); c.block<_8>();							return { c.result, c.i };
		case 25: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_1>();					return { c.result, c.i };
		case 26: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_2>();					return { c.result, c.i };
		case 27: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_2>(); c.block<_1>();			return { c.result, c.i };
		case 28: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_4>();					return { c.result, c.i };
		case 29: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_1>();			return { c.result, c.i };
		case 30: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_2>();			return { c.result, c.i };
		case 31: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_4>(); c.block<_2>(); c.block<_1>();	return { c.result, c.i };
		case 32: c.block<_8>(); c.block<_8>(); c.block<_8>(); c.block<_8>();					return { c.result, c.i };
		}

		__builtin_unreachable();
	}

} // namespace cmp_lcp_impl_



using ResultCmpLCP = cmp_lcp_impl_::ResultLCP;

constexpr ResultCmpLCP cmpLCP(std::string_view a, std::string_view b){
	auto sgn = [](auto a, auto b){
		return (a > b) - (a < b);
	};

	auto const size = std::min(a.size(), b.size());

	if (size == 0)
		return { 0, 0 };

	if (auto const x = cmp_lcp_impl_::cmpMain(a, b, size); x.cmp)
		return x;

	return {
		sgn(a.size(), b.size()),
		size
	};
}

constexpr ResultCmpLCP cmpLCP(std::string_view a, std::string_view b, size_t const start){
	return cmpLCP(
		a.substr(start),
		b.substr(start)
	);
}

constexpr ResultCmpLCP cmpLCPSafe(std::string_view a, std::string_view b, size_t const start){
	return cmpLCP(
		a.size() < start ? "" : a,
		b.size() < start ? "" : b,
		start
	);
}

#if 0
struct PairLCPCompare{
	constexpr PairLCPCompare() = default;

	constexpr PairLCPCompare(size_t val) : prefixL_(val), prefixR_(val){}

	constexpr int operator()(std::string_view a, std::string_view b){
		auto const pr = prefix();

		auto const [comp, pref] = cmpLCP(a, b, pr);

		if (comp >= 0)
			prefixL_ = pr + pref;

		if (comp <= 0)
			prefixR_ = pr + pref;

		return comp;
	}

	constexpr size_t prefix() const{
		return std::min(prefixL_, prefixR_);
	}

private:
	size_t prefixL_ = 0;
	size_t prefixR_ = 0;
};
#endif

#endif

