#ifndef SHARED_ZSET_MULTI_H_
#define SHARED_ZSET_MULTI_H_

#include "mystring.h"
#include "stringtokenizer.h"

namespace net::worker::shared::zsetmulti{
	namespace impl_{
		template<size_t N>
		bool valid(std::array<std::string_view, N> const &indexes){
			for(auto const &x : indexes)
				if (x.empty())
					return false;

			return true;
		}

		template<size_t N>
		auto makeACopy(std::array<std::string_view, N> const &src){
			struct Storage{
				std::array<std::string_view,   N> copy;
				std::array<hm4::PairBufferKey, N> buffer;
			};

			Storage x;

			for(size_t i = 0; i < N; ++i)
				x.copy[i] = concatenateBuffer(x.buffer[i], src[i]);

			return x;
		}

		template<typename Permutation, typename IndexController>
		std::string_view encodeIndex(hm4::PairBufferKey &bufferVal, std::string_view separator, std::array<std::string_view, Permutation::N> const &indexes,
							std::string_view value){
			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				return Permutation::encodeIndex(bufferVal, separator, indexes);
			}else{
				logger<Logger::DEBUG>() << "Using IndexController to encode";
				return IndexController::encode(value);
			}
		}

		template<typename Permutation, typename IndexController>
		std::array<std::string_view, Permutation::N> decodeIndex(std::string_view separator, std::string_view value){
			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				return Permutation::decodeIndex(separator, value);
			}else{
				logger<Logger::DEBUG>() << "Using IndexController to decode";
				return IndexController::template decode<Permutation::N>(value);
			}
		}

		#if 0
		constexpr auto s2u_2(std::string_view s){
			using T = uint16_t;

			auto _ = [s](size_t i){
				return T(s[i]) << (8 * i);
			};

			T const up = 0x2020;

			return _(0) | _(1) | up;
		}

		constexpr auto s2u_3(std::string_view s){
			using T = uint32_t;

			auto _ = [s](size_t i){
				return T(s[i]) << (8 * i);
			};

			T const up = 0x202020;

			return _(0) | _(1) | _(2) | up;
		}

		constexpr auto s2u_4(std::string_view s){
			using T = uint32_t;

			auto _ = [s](size_t i){
				return T(s[i]) << (8 * i);
			};

			T const up = 0x20202020;

			return _(0) | _(1) | _(2) | _(3) | up;
		}
		#endif

	} // namespace impl_



	std::string_view makeKeyCtrl(hm4::PairBufferKey &bufferKey, std::string_view separator,
				std::string_view keyN,
				std::string_view keySub){

		return concatenateBuffer(bufferKey,
				keyN		,	separator	,
							separator	,
				keySub
		);
	}





	struct Permutation1NoIndex{
		constexpr static size_t N = 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~A~keySub, 2 * ~ + 0 * _
			return hm4::Pair::isCompositeKeyValid(2 * 1 + 0 * 1 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~A~keySub, 2 * ~ + 0 * _
			return hm4::Pair::isCompositeKeyValid(2 * 1 + 0 * 1 + more, keyN, keySub, indexes[0]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view /* separator */, std::array<std::string_view, N> const &indexes){
			// no need to copy, but lets do it anyway, because the caller expects it.
			return concatenateBuffer(bufferKey,
						indexes[0]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _() };
		}

		constexpr static size_t sizeKey(std::string_view keyN){
			// keyN~, 1 * ~ + keyN
			return 1 * 1 + keyN.size();
		}

		template<bool LAST_SEPARATOR = true>
		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view /* txt */,
					std::string_view a = "", std::string_view b = ""){

			auto const separator_last = [separator](){
				if constexpr(LAST_SEPARATOR)
					return separator;
				else
					return "";
			}();

			uint64_t const x =
				(a.empty() ? 0x00 : 0xA0) |
				(b.empty() ? 0x00 : 0x0B) |
				0;

			switch(x){
			default:
			case 0x00:
				return concatenateBuffer(bufferKey,
						key	,	separator
				);

			case 0xA0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						a	,	separator_last
				);

			case 0xAB:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						a	,	separator	,
						b
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					indexes[0]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b);

				func(key);
			};

			// old style not supports txt
			ff("X", A, S);
		}
	};



	template<int>
	struct Permutation;



	template<>
	struct Permutation<1>{
		constexpr static size_t N = 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~_~A~keySub, 3 * ~ + _
			return hm4::Pair::isCompositeKeyValid(3 * 1 + 1 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~_~A~keySub, 3 * ~ + _
			return hm4::Pair::isCompositeKeyValid(3 * 1 + 1 + more, keyN, keySub, indexes[0]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view /* separator */, std::array<std::string_view, N> const &indexes){
			// no need to copy, but lets do it anyway, because the caller expects it.
			return concatenateBuffer(bufferKey,
						indexes[0]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = ""){

			uint64_t const x =
				(a.empty() ? 0x00 : 0xA0) |
				(b.empty() ? 0x00 : 0x0B) |
				0;

			switch(x){
			default:
			case 0x00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"A"		,	separator	,
					indexes[0]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b);

				func(key);
			};

			ff("A", A, S);
		}

		#if 0
		static std::string_view validateIndex(std::string_view s){
			if (s.size() != 1)
				return "";

			char const up = 0x20;

			char const x = s[0] | up;

			if (x == 'A')
				return "A";

			return "";
		}
		#endif
	};



	template<>
	struct Permutation<2>{
		constexpr static size_t N = 2;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~AB~A~B~keySub, 4 * ~ + AB
			return hm4::Pair::isCompositeKeyValid(4 * 1 + 2 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~AB~A~B~keySub, 4 * ~ + AB
			return hm4::Pair::isCompositeKeyValid(4 * 1 + 2 + more, keyN, keySub, indexes[0], indexes[1]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = ""){

			uint64_t const x =
				(a.empty() ? 0x000 : 0xA00) |
				(b.empty() ? 0x000 : 0x0B0) |
				(c.empty() ? 0x000 : 0x00C) |
				0;

			switch(x){
			default:
			case 0x000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"AB"		,	separator	,
					indexes[0]	,	separator	,
					indexes[1]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c);

				func(key);
			};

			ff("AB", A, B, S);
			ff("BA", B, A, S);
		}

	public:
		#if 0
		static std::string_view validateIndex(std::string_view s){
			if (s.size() != 2)
				return "";

			auto _ = [](std::string_view s){
				return impl_::s2u_2(s);
			};

			switch( _(s) ){
			case _("AB") : return "AB";
			case _("BA") : return "BA";
			default:
				return "";
			}
		}
		#endif
	};



	template<>
	struct Permutation<3>{
		constexpr static size_t N = 3;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABC~A~B~C~keySub, 5 * ~ + ABC
			return hm4::Pair::isCompositeKeyValid(5 * 1 + 3 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABC~A~B~C~keySub, 5 * ~ + ABC
			return hm4::Pair::isCompositeKeyValid(5 * 1 + 3 + more, keyN, keySub, indexes[0], indexes[1], indexes[2]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
		//	logger<Logger::DEBUG>() << indexes[0] << indexes[1] << indexes[2];

			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1],	separator	,
						indexes[2]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "", std::string_view d = ""){

			uint64_t const x =
				(a.empty() ? 0x0000 : 0xA000) |
				(b.empty() ? 0x0000 : 0x0B00) |
				(c.empty() ? 0x0000 : 0x00C0) |
				(d.empty() ? 0x0000 : 0x000D) |
				0;

			switch(x){
			default:
			case 0x0000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			case 0xABCD:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"ABC"		,	separator	,
					indexes[0]	,	separator	,
					indexes[1]	,	separator	,
					indexes[2]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d);

				func(key);
			};

			ff("ABC", A, B, C, S);
			ff("ACB", A, C, B, S);
			ff("BAC", B, A, C, S);
			ff("BCA", B, C, A, S);
			ff("CAB", C, A, B, S);
			ff("CBA", C, B, A, S);

		}
	};



	template<>
	struct Permutation<4>{
		constexpr static size_t N = 4;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABC~A~B~C~D~keySub, 6 * ~ + ABCD
			return hm4::Pair::isCompositeKeyValid(6 * 1 + 4 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABC~A~B~C~D~keySub, 6 * ~ + ABCD
			return hm4::Pair::isCompositeKeyValid(6 * 1 + 4 + more, keyN, keySub, indexes[0], indexes[1], indexes[2], indexes[3]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
		//	logger<Logger::DEBUG>() << indexes[0] << indexes[1] << indexes[2];

			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1],	separator	,
						indexes[2],	separator	,
						indexes[3]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _(), _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "", std::string_view d = "", std::string_view e = ""){

			uint64_t const x =
				(a.empty() ? 0x00000 : 0xA0000) |
				(b.empty() ? 0x00000 : 0x0B000) |
				(c.empty() ? 0x00000 : 0x00C00) |
				(d.empty() ? 0x00000 : 0x000D0) |
				(e.empty() ? 0x00000 : 0x0000E) |
				0;

			switch(x){
			default:
			case 0x00000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA0000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			case 0xABCD0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator
				);

			case 0xABCDE:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"ABCD"		,	separator	,
					indexes[0]	,	separator	,
					indexes[1]	,	separator	,
					indexes[2]	,	separator	,
					indexes[3]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d, std::string_view e){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e);

				func(key);
			};

			ff("ABCD", A, B, C, D, S);
			ff("ABDC", A, B, D, C, S);
			ff("ACBD", A, C, B, D, S);
			ff("ACDB", A, C, D, B, S);
			ff("ADBC", A, D, B, C, S);
			ff("ADCB", A, D, C, B, S);
			ff("BACD", B, A, C, D, S);
			ff("BADC", B, A, D, C, S);
			ff("BCAD", B, C, A, D, S);
			ff("BCDA", B, C, D, A, S);
			ff("BDAC", B, D, A, C, S);
			ff("BDCA", B, D, C, A, S);
			ff("CABD", C, A, B, D, S);
			ff("CADB", C, A, D, B, S);
			ff("CBAD", C, B, A, D, S);
			ff("CBDA", C, B, D, A, S);
			ff("CDAB", C, D, A, B, S);
			ff("CDBA", C, D, B, A, S);
			ff("DABC", D, A, B, C, S);
			ff("DACB", D, A, C, B, S);
			ff("DBAC", D, B, A, C, S);
			ff("DBCA", D, B, C, A, S);
			ff("DCAB", D, C, A, B, S);
			ff("DCBA", D, C, B, A, S);
		}
	};



	template<>
	struct Permutation<5>{
		constexpr static size_t N = 5;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABC~A~B~C~D~E~keySub, 7 * ~ + ABCDE
			return hm4::Pair::isCompositeKeyValid(7 * 1 + 5 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABC~A~B~C~D~E~keySub, 7 * ~ + ABCDE
			return hm4::Pair::isCompositeKeyValid(7 * 1 + 5 + more, keyN, keySub, indexes[0], indexes[1], indexes[2], indexes[3], indexes[4]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
		//	logger<Logger::DEBUG>() << indexes[0] << indexes[1] << indexes[2];

			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1],	separator	,
						indexes[2],	separator	,
						indexes[3],	separator	,
						indexes[4]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _(), _(), _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "",
					std::string_view d = "", std::string_view e = "", std::string_view f = ""){

			uint64_t const x =
				(a.empty() ? 0x000000 : 0xA00000) |
				(b.empty() ? 0x000000 : 0x0B0000) |
				(c.empty() ? 0x000000 : 0x00C000) |
				(d.empty() ? 0x000000 : 0x000D00) |
				(e.empty() ? 0x000000 : 0x0000E0) |
				(f.empty() ? 0x000000 : 0x00000F) |
				0;

			switch(x){
			default:
			case 0x000000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA00000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB0000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			case 0xABCD00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator
				);

			case 0xABCDE0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator
				);

			case 0xABCDEF:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator	,
						f
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"ABCDE"		,	separator	,
					indexes[0]	,	separator	,
					indexes[1]	,	separator	,
					indexes[2]	,	separator	,
					indexes[3]	,	separator	,
					indexes[4]	,	separator	,
					keySub
			);
		}

		template<typename F>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, F func){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];
			auto const E = indexes[4];

			auto ff = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e, std::string_view f){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, f);

				func(key);
			};

			ff("ABCDE", A, B, C, D, E, S);
			ff("ABCED", A, B, C, E, D, S);
			ff("ABDCE", A, B, D, C, E, S);
			ff("ABDEC", A, B, D, E, C, S);
			ff("ABECD", A, B, E, C, D, S);
			ff("ABEDC", A, B, E, D, C, S);
			ff("ACBDE", A, C, B, D, E, S);
			ff("ACBED", A, C, B, E, D, S);
			ff("ACDBE", A, C, D, B, E, S);
			ff("ACDEB", A, C, D, E, B, S);
			ff("ACEBD", A, C, E, B, D, S);
			ff("ACEDC", A, C, E, D, C, S);
			ff("ADBCE", A, D, B, C, E, S);
			ff("ADBEC", A, D, B, E, C, S);
			ff("ADCBE", A, D, C, B, E, S);
			ff("ADCEB", A, D, C, E, B, S);
			ff("ADEBC", A, D, E, B, C, S);
			ff("ADECB", A, D, E, C, B, S);
			ff("AEBCD", A, E, B, C, D, S);
			ff("AEBDC", A, E, B, D, C, S);
			ff("AECBD", A, E, C, B, D, S);
			ff("AECDB", A, E, C, D, B, S);
			ff("AEDBC", A, E, D, B, C, S);
			ff("AEDCB", A, E, D, C, B, S);
			ff("BACDE", B, A, C, D, E, S);
			ff("BACED", B, A, C, E, D, S);
			ff("BADCE", B, A, D, C, E, S);
			ff("BADEC", B, A, D, E, C, S);
			ff("BAECD", B, A, E, C, D, S);
			ff("BAEDC", B, A, E, D, C, S);
			ff("BCADE", B, C, A, D, E, S);
			ff("BCAED", B, C, A, E, D, S);
			ff("BCDAE", B, C, D, A, E, S);
			ff("BCDEA", B, C, D, E, A, S);
			ff("BCEAD", B, C, E, A, D, S);
			ff("BCEDA", B, C, E, D, A, S);
			ff("BDACE", B, D, A, C, E, S);
			ff("BDAEC", B, D, A, E, C, S);
			ff("BDCAE", B, D, C, A, E, S);
			ff("BDCEA", B, D, C, E, A, S);
			ff("BDEAC", B, D, E, A, C, S);
			ff("BDECA", B, D, E, C, A, S);
			ff("BEACD", B, E, A, C, D, S);
			ff("BEADC", B, E, A, D, C, S);
			ff("BECAD", B, E, C, A, D, S);
			ff("BECDA", B, E, C, D, A, S);
			ff("BEDAC", B, E, D, A, C, S);
			ff("BEDCA", B, E, D, C, A, S);
			ff("CABDE", C, A, B, D, E, S);
			ff("CABED", C, A, B, E, D, S);
			ff("CADBE", C, A, D, B, E, S);
			ff("CADEB", C, A, D, E, B, S);
			ff("CAEBD", C, A, E, B, D, S);
			ff("CAEDB", C, A, E, D, B, S);
			ff("CBADE", C, B, A, D, E, S);
			ff("CBAED", C, B, A, E, D, S);
			ff("CBDAE", C, B, D, A, E, S);
			ff("CBDEA", C, B, D, E, A, S);
			ff("CBEAD", C, B, E, A, D, S);
			ff("CBEDA", C, B, E, D, A, S);
			ff("CDABE", C, D, A, B, E, S);
			ff("CDAEB", C, D, A, E, B, S);
			ff("CDBAE", C, D, B, A, E, S);
			ff("CDBEA", C, D, B, E, A, S);
			ff("CDEAB", C, D, E, A, B, S);
			ff("CDEBA", C, D, E, B, A, S);
			ff("CEABD", C, E, A, B, D, S);
			ff("CEADB", C, E, A, D, B, S);
			ff("CEBAD", C, E, B, A, D, S);
			ff("CEBDA", C, E, B, D, A, S);
			ff("CEDAB", C, E, D, A, B, S);
			ff("CEDBA", C, E, D, B, A, S);
			ff("DABCE", D, A, B, C, E, S);
			ff("DABEC", D, A, B, E, C, S);
			ff("DACBE", D, A, C, B, E, S);
			ff("DACEB", D, A, C, E, B, S);
			ff("DAEBC", D, A, E, B, C, S);
			ff("DAECB", D, A, E, C, B, S);
			ff("DBACE", D, B, A, C, E, S);
			ff("DBAEC", D, B, A, E, C, S);
			ff("DBCAE", D, B, C, A, E, S);
			ff("DBCEA", D, B, C, E, A, S);
			ff("DBEAC", D, B, E, A, C, S);
			ff("DBECA", D, B, E, C, A, S);
			ff("DCABE", D, C, A, B, E, S);
			ff("DCAEB", D, C, A, E, B, S);
			ff("DCBAE", D, C, B, A, E, S);
			ff("DCBEA", D, C, B, E, A, S);
			ff("DCEAB", D, C, E, A, B, S);
			ff("DCEBA", D, C, E, B, A, S);
			ff("DEABC", D, E, A, B, C, S);
			ff("DEACB", D, E, A, C, B, S);
			ff("DEBAC", D, E, B, A, C, S);
			ff("DEBCA", D, E, B, C, A, S);
			ff("DECAB", D, E, C, A, B, S);
			ff("DECBA", D, E, C, B, A, S);
			ff("EABCD", E, A, B, C, D, S);
			ff("EABDC", E, A, B, D, C, S);
			ff("EACBD", E, A, C, B, D, S);
			ff("EACDB", E, A, C, D, B, S);
			ff("EADBC", E, A, D, B, C, S);
			ff("EADCB", E, A, D, C, B, S);
			ff("EBACD", E, B, A, C, D, S);
			ff("EBADC", E, B, A, D, C, S);
			ff("EBCAD", E, B, C, A, D, S);
			ff("EBCDA", E, B, C, D, A, S);
			ff("EBDAC", E, B, D, A, C, S);
			ff("EBDCA", E, B, D, C, A, S);
			ff("ECABD", E, C, A, B, D, S);
			ff("ECADB", E, C, A, D, B, S);
			ff("ECBAD", E, C, B, A, D, S);
			ff("ECBDA", E, C, B, D, A, S);
			ff("ECDAB", E, C, D, A, B, S);
			ff("ECDBA", E, C, D, B, A, S);
			ff("EDABC", E, D, A, B, C, S);
			ff("EDACB", E, D, A, C, B, S);
			ff("EDBAC", E, D, B, A, C, S);
			ff("EDBCA", E, D, B, C, A, S);
			ff("EDCAB", E, D, C, A, B, S);
			ff("EDCBA", E, D, C, B, A, S);
		}
	};



	template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::array<std::string_view, Permutation::N> const &indexes, std::string_view value){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: ctrl key" << keyCtrl;

		{
			// Update control key and delete hash key if any

			if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
				// Case 1: ctrl key is set

				auto const indexesOld = decodeIndex<Permutation, IndexController>(DBAdapter::SEPARATOR, pair->getVal());

				if (!valid(indexesOld)){
					// Case 1.0: invalid ctrl key, probable attack.

					logger<Logger::DEBUG>() << "ZSetMulti::ADD: INVALID ctrl key" << keyCtrl;

					// HINT
					const auto *hint = pair;
					hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);
				}else if (indexesOld == indexes){
					// Case 1.1: ctrl key is same, no need to be updated.

					logger<Logger::DEBUG>() << "ZSetMulti::ADD: SKIP SET ctrl key" << keyCtrl;
				}else{
					// Case 1.2: old ctrl key has to be updated

					auto const save = makeACopy(indexesOld);

					auto deleteOldKeys = [&](std::string_view key){
						logger<Logger::DEBUG>() << "ZSetMulti::ADD: DEL old index key" << key;

						erase(*db, key);
					};

					Permutation::template for_each(DBAdapter::SEPARATOR, keyN, keySub, save.copy, deleteOldKeys);
				}
			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "ZSetMulti::ADD: no ctrl key" << keyCtrl;
			}
		}

		auto insertNewKeys = [&](std::string_view key){
			logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET index key" << key;

			insert(*db, key, value);
		};

		Permutation::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexes, insertNewKeys);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET ctrl key" << keyCtrl;

		hm4::PairBufferKey bufferVal;

		auto const encodedValue = encodeIndex<Permutation, IndexController>(bufferVal, DBAdapter::SEPARATOR, indexes, value);

		insert(*db, keyCtrl, encodedValue);
	}



	template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
	void rem(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::REM: ctrl key" << keyCtrl;

		if (auto const pair = hm4::getPairPtr(*db, keyCtrl); pair){
			// Case 1: ctrl key is set

			auto const indexesOld = decodeIndex<Permutation, IndexController>(DBAdapter::SEPARATOR, pair->getVal());

			if (!valid(indexesOld)){
				// Case 1.0: invalid ctrl key, probable attack.

				logger<Logger::DEBUG>() << "ZSetMulti::REM: INVALID ctrl key" << keyCtrl;

				// HINT
				const auto *hint = pair;
				hm4::insertHintF<hm4::PairFactory::Tombstone>(*db, hint, keyCtrl);
			}else{
				// Case 1.1: old ctrl key has to be removed

				hm4::PairBufferKey bufferKeySave[Permutation::N];

				// make a copy, because values may be invalidated.
				using ArrayN = std::array<std::string_view, Permutation::N>;

				ArrayN indexesOldSave;

				for(size_t i = 0; i < Permutation::N; ++i)
					indexesOldSave[i] = concatenateBuffer(bufferKeySave[i], indexesOld[i]);

				auto deleteOldKeys = [&](std::string_view key){
					logger<Logger::DEBUG>() << "ZSetMulti::REM: DEL old index key" << key;

					erase(*db, key);
				};

				Permutation::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexesOldSave, deleteOldKeys);

				logger<Logger::DEBUG>() << "ZSetMulti::REM: DEL ctrl key" << keyCtrl;

				erase(*db, keyCtrl);
			}
		}else{
			// Case 2: no ctrl key

			logger<Logger::DEBUG>() << "ZSetMulti::REM: no ctrl key" << keyCtrl;
		}
	}



	template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
	std::string_view get(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::GET: ctrl key" << keyCtrl;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
			// Case 1: ctrl key is set

			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				using namespace impl_;

				auto const indexes = decodeIndex<Permutation, IndexController>(DBAdapter::SEPARATOR, encodedValue);

				if (Permutation::valid(keyN, keySub, indexes)){
					hm4::PairBufferKey bufferKeyData;
					auto const keyData = Permutation::makeKeyData(bufferKeyData, DBAdapter::SEPARATOR, keyN, keySub, indexes);

					logger<Logger::DEBUG>() << "ZSetMulti::GET: data key" << keyData;

					return hm4::getPairVal(*db, keyData);
				}
			}else{
				logger<Logger::DEBUG>() << "Using IndexController to get data";
				return IndexController::decodeValue(encodedValue);
			}
		}

		return "";
	}



	template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
	std::array<std::string_view, Permutation::N> getIndexes(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::GET_INDEX: ctrl key" << keyCtrl;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
			// Case 1: ctrl key is set

			using namespace impl_;

			auto const indexes = decodeIndex<Permutation, IndexController>(DBAdapter::SEPARATOR, encodedValue);

			if (Permutation::valid(keyN, keySub, indexes))
				return indexes;
		}

		return {};
	}



	template<typename DBAdapter>
	bool exists(DBAdapter &db,
			std::string_view keyN, std::string_view keySub){

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::EXISTS: ctrl key" << keyCtrl;

		return hm4::getPairOK(*db, keyCtrl);
	}



	template<typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
		// EXISTS key subkey0

		if (p.size() != 3)
			return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

		auto const &keyN   = p[1];
		auto const &keySub = p[2];

		if (keyN.empty() || keySub.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		if (!Permutation<1>::valid(keyN, keySub))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		return result.set(
			exists(db, keyN, keySub)
		);
	}



	template<typename Permutation, typename IndexController = std::nullptr_t, typename ParamContainer, typename OutputBlob, typename Result, typename DBAdapter>
	void cmdProcessRem(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
		// REM key subkey0 subkey1 ...

		if (p.size() < 3)
			return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

		const auto &keyN = p[1];

		if (keyN.empty())
			return result.set_error(ResultErrorMessages::EMPTY_KEY);

		auto const varg = 2;

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			if (auto const &keySub = *itk; !Permutation::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
		}

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			auto const &keySub = *itk;

			rem<Permutation, IndexController>(db, keyN, keySub);
		}

		return result.set_1();
	}

} // net::worker::shared::zset

#endif

