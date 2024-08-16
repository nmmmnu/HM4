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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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
			// keyN~ABCD~A~B~C~D~keySub, 6 * ~ + ABCD
			return hm4::Pair::isCompositeKeyValid(6 * 1 + 4 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABCD~A~B~C~D~keySub, 6 * ~ + ABCD
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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
			// keyN~ABCDE~A~B~C~D~E~keySub, 7 * ~ + ABCDE
			return hm4::Pair::isCompositeKeyValid(7 * 1 + 5 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABCDE~A~B~C~D~E~keySub, 7 * ~ + ABCDE
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
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

			// All 120 permutations here:
			// thanks chatgpt

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



	template<>
	struct Permutation<6>{
		constexpr static size_t N = 6;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABCDEF~A~B~C~D~E~F~keySub, 8 * ~ + ABCDEF
			return hm4::Pair::isCompositeKeyValid(8 * 1 + 6 + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABCDEF~A~B~C~D~E~F~keySub, 8 * ~ + ABCDEF
			return hm4::Pair::isCompositeKeyValid(8 * 1 + 6 + more, keyN, keySub, indexes[0], indexes[1], indexes[2], indexes[3], indexes[4]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
		//	logger<Logger::DEBUG>() << indexes[0] << indexes[1] << indexes[2];

			return concatenateBuffer(bufferKey,
						indexes[0],	separator	,
						indexes[1],	separator	,
						indexes[2],	separator	,
						indexes[3],	separator	,
						indexes[4],	separator	,
						indexes[5]
			);
		}

		static auto decodeIndex(std::string_view separator, std::string_view s){
			StringTokenizer const tok{ s, separator[0] };
			auto _ = getForwardTokenizer(tok);

			return std::array<std::string_view, N>{ _(), _(), _(), _(), _(), _() };
		}

		static std::string_view makeKey(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "",
					std::string_view d = "", std::string_view e = "", std::string_view f = "",
					std::string_view g = ""){

			uint64_t const x =
				(a.empty() ? 0x0000000 : 0xA000000) |
				(b.empty() ? 0x0000000 : 0x0B00000) |
				(c.empty() ? 0x0000000 : 0x00C0000) |
				(d.empty() ? 0x0000000 : 0x000D000) |
				(e.empty() ? 0x0000000 : 0x0000E00) |
				(f.empty() ? 0x0000000 : 0x00000F0) |
				(f.empty() ? 0x0000000 : 0x0000001) |
				0;

			switch(x){
			default:
			case 0x0000000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			case 0xA000000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			case 0xAB00000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			case 0xABC0000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			case 0xABCD000:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator
				);

			case 0xABCDE00:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator
				);

			case 0xABCDEF0:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator	,
						f	,	separator
				);

			case 0xABCDEF1:
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator	,
						f	,	separator	,
						g
				);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return concatenateBuffer(bufferKey,
					keyN		,	separator	,
					"ABCDEF"	,	separator	,
					indexes[0]	,	separator	,
					indexes[1]	,	separator	,
					indexes[2]	,	separator	,
					indexes[3]	,	separator	,
					indexes[4]	,	separator	,
					indexes[5]	,	separator	,
					keySub
			);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const S = keySub;
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];
			auto const E = indexes[4];
			auto const F = indexes[5];

			auto ff = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e, std::string_view f,
							std::string_view g){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, f, g);

				func(key);
			};

			// All 720 permutations here:
			// thanks wolfram alpha
			// this is still acceptable, sinse XNDEL can do 32K inserts.

			ff("ABCDEF", A, B, C, D, E, F, S);	ff("CABDEF", C, A, B, D, E, F, S);	ff("EABCDF", E, A, B, C, D, F, S);
			ff("ABCDFE", A, B, C, D, F, E, S);	ff("CABDFE", C, A, B, D, F, E, S);	ff("EABCFD", E, A, B, C, F, D, S);
			ff("ABCEDF", A, B, C, E, D, F, S);	ff("CABEDF", C, A, B, E, D, F, S);	ff("EABDCF", E, A, B, D, C, F, S);
			ff("ABCEFD", A, B, C, E, F, D, S);	ff("CABEFD", C, A, B, E, F, D, S);	ff("EABDFC", E, A, B, D, F, C, S);
			ff("ABCFDE", A, B, C, F, D, E, S);	ff("CABFDE", C, A, B, F, D, E, S);	ff("EABFCD", E, A, B, F, C, D, S);
			ff("ABCFED", A, B, C, F, E, D, S);	ff("CABFED", C, A, B, F, E, D, S);	ff("EABFDC", E, A, B, F, D, C, S);
			ff("ABDCEF", A, B, D, C, E, F, S);	ff("CADBEF", C, A, D, B, E, F, S);	ff("EACBDF", E, A, C, B, D, F, S);
			ff("ABDCFE", A, B, D, C, F, E, S);	ff("CADBFE", C, A, D, B, F, E, S);	ff("EACBFD", E, A, C, B, F, D, S);
			ff("ABDECF", A, B, D, E, C, F, S);	ff("CADEBF", C, A, D, E, B, F, S);	ff("EACDBF", E, A, C, D, B, F, S);
			ff("ABDEFC", A, B, D, E, F, C, S);	ff("CADEFB", C, A, D, E, F, B, S);	ff("EACDFB", E, A, C, D, F, B, S);
			ff("ABDFCE", A, B, D, F, C, E, S);	ff("CADFBE", C, A, D, F, B, E, S);	ff("EACFBD", E, A, C, F, B, D, S);
			ff("ABDFEC", A, B, D, F, E, C, S);	ff("CADFEB", C, A, D, F, E, B, S);	ff("EACFDB", E, A, C, F, D, B, S);
			ff("ABECDF", A, B, E, C, D, F, S);	ff("CAEBDF", C, A, E, B, D, F, S);	ff("EADBCF", E, A, D, B, C, F, S);
			ff("ABECFD", A, B, E, C, F, D, S);	ff("CAEBFD", C, A, E, B, F, D, S);	ff("EADBFC", E, A, D, B, F, C, S);
			ff("ABEDCF", A, B, E, D, C, F, S);	ff("CAEDBF", C, A, E, D, B, F, S);	ff("EADCBF", E, A, D, C, B, F, S);
			ff("ABEDFC", A, B, E, D, F, C, S);	ff("CAEDFB", C, A, E, D, F, B, S);	ff("EADCFB", E, A, D, C, F, B, S);
			ff("ABEFCD", A, B, E, F, C, D, S);	ff("CAEFBD", C, A, E, F, B, D, S);	ff("EADFBC", E, A, D, F, B, C, S);
			ff("ABEFDC", A, B, E, F, D, C, S);	ff("CAEFDB", C, A, E, F, D, B, S);	ff("EADFCB", E, A, D, F, C, B, S);
			ff("ABFCDE", A, B, F, C, D, E, S);	ff("CAFBDE", C, A, F, B, D, E, S);	ff("EAFBCD", E, A, F, B, C, D, S);
			ff("ABFCED", A, B, F, C, E, D, S);	ff("CAFBED", C, A, F, B, E, D, S);	ff("EAFBDC", E, A, F, B, D, C, S);
			ff("ABFDCE", A, B, F, D, C, E, S);	ff("CAFDBE", C, A, F, D, B, E, S);	ff("EAFCBD", E, A, F, C, B, D, S);
			ff("ABFDEC", A, B, F, D, E, C, S);	ff("CAFDEB", C, A, F, D, E, B, S);	ff("EAFCDB", E, A, F, C, D, B, S);
			ff("ABFECD", A, B, F, E, C, D, S);	ff("CAFEBD", C, A, F, E, B, D, S);	ff("EAFDBC", E, A, F, D, B, C, S);
			ff("ABFEDC", A, B, F, E, D, C, S);	ff("CAFEDB", C, A, F, E, D, B, S);	ff("EAFDCB", E, A, F, D, C, B, S);
			ff("ACBDEF", A, C, B, D, E, F, S);	ff("CBADEF", C, B, A, D, E, F, S);	ff("EBACDF", E, B, A, C, D, F, S);
			ff("ACBDFE", A, C, B, D, F, E, S);	ff("CBADFE", C, B, A, D, F, E, S);	ff("EBACFD", E, B, A, C, F, D, S);
			ff("ACBEDF", A, C, B, E, D, F, S);	ff("CBAEDF", C, B, A, E, D, F, S);	ff("EBADCF", E, B, A, D, C, F, S);
			ff("ACBEFD", A, C, B, E, F, D, S);	ff("CBAEFD", C, B, A, E, F, D, S);	ff("EBADFC", E, B, A, D, F, C, S);
			ff("ACBFDE", A, C, B, F, D, E, S);	ff("CBAFDE", C, B, A, F, D, E, S);	ff("EBAFCD", E, B, A, F, C, D, S);
			ff("ACBFED", A, C, B, F, E, D, S);	ff("CBAFED", C, B, A, F, E, D, S);	ff("EBAFDC", E, B, A, F, D, C, S);
			ff("ACDBEF", A, C, D, B, E, F, S);	ff("CBDAEF", C, B, D, A, E, F, S);	ff("EBCADF", E, B, C, A, D, F, S);
			ff("ACDBFE", A, C, D, B, F, E, S);	ff("CBDAFE", C, B, D, A, F, E, S);	ff("EBCAFD", E, B, C, A, F, D, S);
			ff("ACDEBF", A, C, D, E, B, F, S);	ff("CBDEAF", C, B, D, E, A, F, S);	ff("EBCDAF", E, B, C, D, A, F, S);
			ff("ACDEFB", A, C, D, E, F, B, S);	ff("CBDEFA", C, B, D, E, F, A, S);	ff("EBCDFA", E, B, C, D, F, A, S);
			ff("ACDFBE", A, C, D, F, B, E, S);	ff("CBDFAE", C, B, D, F, A, E, S);	ff("EBCFAD", E, B, C, F, A, D, S);
			ff("ACDFEB", A, C, D, F, E, B, S);	ff("CBDFEA", C, B, D, F, E, A, S);	ff("EBCFDA", E, B, C, F, D, A, S);
			ff("ACEBDF", A, C, E, B, D, F, S);	ff("CBEADF", C, B, E, A, D, F, S);	ff("EBDACF", E, B, D, A, C, F, S);
			ff("ACEBFD", A, C, E, B, F, D, S);	ff("CBEAFD", C, B, E, A, F, D, S);	ff("EBDAFC", E, B, D, A, F, C, S);
			ff("ACEDBF", A, C, E, D, B, F, S);	ff("CBEDAF", C, B, E, D, A, F, S);	ff("EBDCAF", E, B, D, C, A, F, S);
			ff("ACEDFB", A, C, E, D, F, B, S);	ff("CBEDFA", C, B, E, D, F, A, S);	ff("EBDCFA", E, B, D, C, F, A, S);
			ff("ACEFBD", A, C, E, F, B, D, S);	ff("CBEFAD", C, B, E, F, A, D, S);	ff("EBDFAC", E, B, D, F, A, C, S);
			ff("ACEFDB", A, C, E, F, D, B, S);	ff("CBEFDA", C, B, E, F, D, A, S);	ff("EBDFCA", E, B, D, F, C, A, S);
			ff("ACFBDE", A, C, F, B, D, E, S);	ff("CBFADE", C, B, F, A, D, E, S);	ff("EBFACD", E, B, F, A, C, D, S);
			ff("ACFBED", A, C, F, B, E, D, S);	ff("CBFAED", C, B, F, A, E, D, S);	ff("EBFADC", E, B, F, A, D, C, S);
			ff("ACFDBE", A, C, F, D, B, E, S);	ff("CBFDAE", C, B, F, D, A, E, S);	ff("EBFCAD", E, B, F, C, A, D, S);
			ff("ACFDEB", A, C, F, D, E, B, S);	ff("CBFDEA", C, B, F, D, E, A, S);	ff("EBFCDA", E, B, F, C, D, A, S);
			ff("ACFEBD", A, C, F, E, B, D, S);	ff("CBFEAD", C, B, F, E, A, D, S);	ff("EBFDAC", E, B, F, D, A, C, S);
			ff("ACFEDB", A, C, F, E, D, B, S);	ff("CBFEDA", C, B, F, E, D, A, S);	ff("EBFDCA", E, B, F, D, C, A, S);
			ff("ADBCEF", A, D, B, C, E, F, S);	ff("CDABEF", C, D, A, B, E, F, S);	ff("ECABDF", E, C, A, B, D, F, S);
			ff("ADBCFE", A, D, B, C, F, E, S);	ff("CDABFE", C, D, A, B, F, E, S);	ff("ECABFD", E, C, A, B, F, D, S);
			ff("ADBECF", A, D, B, E, C, F, S);	ff("CDAEBF", C, D, A, E, B, F, S);	ff("ECADBF", E, C, A, D, B, F, S);
			ff("ADBEFC", A, D, B, E, F, C, S);	ff("CDAEFB", C, D, A, E, F, B, S);	ff("ECADFB", E, C, A, D, F, B, S);
			ff("ADBFCE", A, D, B, F, C, E, S);	ff("CDAFBE", C, D, A, F, B, E, S);	ff("ECAFBD", E, C, A, F, B, D, S);
			ff("ADBFEC", A, D, B, F, E, C, S);	ff("CDAFEB", C, D, A, F, E, B, S);	ff("ECAFDB", E, C, A, F, D, B, S);
			ff("ADCBEF", A, D, C, B, E, F, S);	ff("CDBAEF", C, D, B, A, E, F, S);	ff("ECBADF", E, C, B, A, D, F, S);
			ff("ADCBFE", A, D, C, B, F, E, S);	ff("CDBAFE", C, D, B, A, F, E, S);	ff("ECBAFD", E, C, B, A, F, D, S);
			ff("ADCEBF", A, D, C, E, B, F, S);	ff("CDBEAF", C, D, B, E, A, F, S);	ff("ECBDAF", E, C, B, D, A, F, S);
			ff("ADCEFB", A, D, C, E, F, B, S);	ff("CDBEFA", C, D, B, E, F, A, S);	ff("ECBDFA", E, C, B, D, F, A, S);
			ff("ADCFBE", A, D, C, F, B, E, S);	ff("CDBFAE", C, D, B, F, A, E, S);	ff("ECBFAD", E, C, B, F, A, D, S);
			ff("ADCFEB", A, D, C, F, E, B, S);	ff("CDBFEA", C, D, B, F, E, A, S);	ff("ECBFDA", E, C, B, F, D, A, S);
			ff("ADEBCF", A, D, E, B, C, F, S);	ff("CDEABF", C, D, E, A, B, F, S);	ff("ECDABF", E, C, D, A, B, F, S);
			ff("ADEBFC", A, D, E, B, F, C, S);	ff("CDEAFB", C, D, E, A, F, B, S);	ff("ECDAFB", E, C, D, A, F, B, S);
			ff("ADECBF", A, D, E, C, B, F, S);	ff("CDEBAF", C, D, E, B, A, F, S);	ff("ECDBAF", E, C, D, B, A, F, S);
			ff("ADECFB", A, D, E, C, F, B, S);	ff("CDEBFA", C, D, E, B, F, A, S);	ff("ECDBFA", E, C, D, B, F, A, S);
			ff("ADEFBC", A, D, E, F, B, C, S);	ff("CDEFAB", C, D, E, F, A, B, S);	ff("ECDFAB", E, C, D, F, A, B, S);
			ff("ADEFCB", A, D, E, F, C, B, S);	ff("CDEFBA", C, D, E, F, B, A, S);	ff("ECDFBA", E, C, D, F, B, A, S);
			ff("ADFBCE", A, D, F, B, C, E, S);	ff("CDFABE", C, D, F, A, B, E, S);	ff("ECFABD", E, C, F, A, B, D, S);
			ff("ADFBEC", A, D, F, B, E, C, S);	ff("CDFAEB", C, D, F, A, E, B, S);	ff("ECFADB", E, C, F, A, D, B, S);
			ff("ADFCBE", A, D, F, C, B, E, S);	ff("CDFBAE", C, D, F, B, A, E, S);	ff("ECFBAD", E, C, F, B, A, D, S);
			ff("ADFCEB", A, D, F, C, E, B, S);	ff("CDFBEA", C, D, F, B, E, A, S);	ff("ECFBDA", E, C, F, B, D, A, S);
			ff("ADFEBC", A, D, F, E, B, C, S);	ff("CDFEAB", C, D, F, E, A, B, S);	ff("ECFDAB", E, C, F, D, A, B, S);
			ff("ADFECB", A, D, F, E, C, B, S);	ff("CDFEBA", C, D, F, E, B, A, S);	ff("ECFDBA", E, C, F, D, B, A, S);
			ff("AEBCDF", A, E, B, C, D, F, S);	ff("CEABDF", C, E, A, B, D, F, S);	ff("EDABCF", E, D, A, B, C, F, S);
			ff("AEBCFD", A, E, B, C, F, D, S);	ff("CEABFD", C, E, A, B, F, D, S);	ff("EDABFC", E, D, A, B, F, C, S);
			ff("AEBDCF", A, E, B, D, C, F, S);	ff("CEADBF", C, E, A, D, B, F, S);	ff("EDACBF", E, D, A, C, B, F, S);
			ff("AEBDFC", A, E, B, D, F, C, S);	ff("CEADFB", C, E, A, D, F, B, S);	ff("EDACFB", E, D, A, C, F, B, S);
			ff("AEBFCD", A, E, B, F, C, D, S);	ff("CEAFBD", C, E, A, F, B, D, S);	ff("EDAFBC", E, D, A, F, B, C, S);
			ff("AEBFDC", A, E, B, F, D, C, S);	ff("CEAFDB", C, E, A, F, D, B, S);	ff("EDAFCB", E, D, A, F, C, B, S);
			ff("AECBDF", A, E, C, B, D, F, S);	ff("CEBADF", C, E, B, A, D, F, S);	ff("EDBACF", E, D, B, A, C, F, S);
			ff("AECBFD", A, E, C, B, F, D, S);	ff("CEBAFD", C, E, B, A, F, D, S);	ff("EDBAFC", E, D, B, A, F, C, S);
			ff("AECDBF", A, E, C, D, B, F, S);	ff("CEBDAF", C, E, B, D, A, F, S);	ff("EDBCAF", E, D, B, C, A, F, S);
			ff("AECDFB", A, E, C, D, F, B, S);	ff("CEBDFA", C, E, B, D, F, A, S);	ff("EDBCFA", E, D, B, C, F, A, S);
			ff("AECFBD", A, E, C, F, B, D, S);	ff("CEBFAD", C, E, B, F, A, D, S);	ff("EDBFAC", E, D, B, F, A, C, S);
			ff("AECFDB", A, E, C, F, D, B, S);	ff("CEBFDA", C, E, B, F, D, A, S);	ff("EDBFCA", E, D, B, F, C, A, S);
			ff("AEDBCF", A, E, D, B, C, F, S);	ff("CEDABF", C, E, D, A, B, F, S);	ff("EDCABF", E, D, C, A, B, F, S);
			ff("AEDBFC", A, E, D, B, F, C, S);	ff("CEDAFB", C, E, D, A, F, B, S);	ff("EDCAFB", E, D, C, A, F, B, S);
			ff("AEDCBF", A, E, D, C, B, F, S);	ff("CEDBAF", C, E, D, B, A, F, S);	ff("EDCBAF", E, D, C, B, A, F, S);
			ff("AEDCFB", A, E, D, C, F, B, S);	ff("CEDBFA", C, E, D, B, F, A, S);	ff("EDCBFA", E, D, C, B, F, A, S);
			ff("AEDFBC", A, E, D, F, B, C, S);	ff("CEDFAB", C, E, D, F, A, B, S);	ff("EDCFAB", E, D, C, F, A, B, S);
			ff("AEDFCB", A, E, D, F, C, B, S);	ff("CEDFBA", C, E, D, F, B, A, S);	ff("EDCFBA", E, D, C, F, B, A, S);
			ff("AEFBCD", A, E, F, B, C, D, S);	ff("CEFABD", C, E, F, A, B, D, S);	ff("EDFABC", E, D, F, A, B, C, S);
			ff("AEFBDC", A, E, F, B, D, C, S);	ff("CEFADB", C, E, F, A, D, B, S);	ff("EDFACB", E, D, F, A, C, B, S);
			ff("AEFCBD", A, E, F, C, B, D, S);	ff("CEFBAD", C, E, F, B, A, D, S);	ff("EDFBAC", E, D, F, B, A, C, S);
			ff("AEFCDB", A, E, F, C, D, B, S);	ff("CEFBDA", C, E, F, B, D, A, S);	ff("EDFBCA", E, D, F, B, C, A, S);
			ff("AEFDBC", A, E, F, D, B, C, S);	ff("CEFDAB", C, E, F, D, A, B, S);	ff("EDFCAB", E, D, F, C, A, B, S);
			ff("AEFDCB", A, E, F, D, C, B, S);	ff("CEFDBA", C, E, F, D, B, A, S);	ff("EDFCBA", E, D, F, C, B, A, S);
			ff("AFBCDE", A, F, B, C, D, E, S);	ff("CFABDE", C, F, A, B, D, E, S);	ff("EFABCD", E, F, A, B, C, D, S);
			ff("AFBCED", A, F, B, C, E, D, S);	ff("CFABED", C, F, A, B, E, D, S);	ff("EFABDC", E, F, A, B, D, C, S);
			ff("AFBDCE", A, F, B, D, C, E, S);	ff("CFADBE", C, F, A, D, B, E, S);	ff("EFACBD", E, F, A, C, B, D, S);
			ff("AFBDEC", A, F, B, D, E, C, S);	ff("CFADEB", C, F, A, D, E, B, S);	ff("EFACDB", E, F, A, C, D, B, S);
			ff("AFBECD", A, F, B, E, C, D, S);	ff("CFAEBD", C, F, A, E, B, D, S);	ff("EFADBC", E, F, A, D, B, C, S);
			ff("AFBEDC", A, F, B, E, D, C, S);	ff("CFAEDB", C, F, A, E, D, B, S);	ff("EFADCB", E, F, A, D, C, B, S);
			ff("AFCBDE", A, F, C, B, D, E, S);	ff("CFBADE", C, F, B, A, D, E, S);	ff("EFBACD", E, F, B, A, C, D, S);
			ff("AFCBED", A, F, C, B, E, D, S);	ff("CFBAED", C, F, B, A, E, D, S);	ff("EFBADC", E, F, B, A, D, C, S);
			ff("AFCDBE", A, F, C, D, B, E, S);	ff("CFBDAE", C, F, B, D, A, E, S);	ff("EFBCAD", E, F, B, C, A, D, S);
			ff("AFCDEB", A, F, C, D, E, B, S);	ff("CFBDEA", C, F, B, D, E, A, S);	ff("EFBCDA", E, F, B, C, D, A, S);
			ff("AFCEBD", A, F, C, E, B, D, S);	ff("CFBEAD", C, F, B, E, A, D, S);	ff("EFBDAC", E, F, B, D, A, C, S);
			ff("AFCEDB", A, F, C, E, D, B, S);	ff("CFBEDA", C, F, B, E, D, A, S);	ff("EFBDCA", E, F, B, D, C, A, S);
			ff("AFDBCE", A, F, D, B, C, E, S);	ff("CFDABE", C, F, D, A, B, E, S);	ff("EFCABD", E, F, C, A, B, D, S);
			ff("AFDBEC", A, F, D, B, E, C, S);	ff("CFDAEB", C, F, D, A, E, B, S);	ff("EFCADB", E, F, C, A, D, B, S);
			ff("AFDCBE", A, F, D, C, B, E, S);	ff("CFDBAE", C, F, D, B, A, E, S);	ff("EFCBAD", E, F, C, B, A, D, S);
			ff("AFDCEB", A, F, D, C, E, B, S);	ff("CFDBEA", C, F, D, B, E, A, S);	ff("EFCBDA", E, F, C, B, D, A, S);
			ff("AFDEBC", A, F, D, E, B, C, S);	ff("CFDEAB", C, F, D, E, A, B, S);	ff("EFCDAB", E, F, C, D, A, B, S);
			ff("AFDECB", A, F, D, E, C, B, S);	ff("CFDEBA", C, F, D, E, B, A, S);	ff("EFCDBA", E, F, C, D, B, A, S);
			ff("AFEBCD", A, F, E, B, C, D, S);	ff("CFEABD", C, F, E, A, B, D, S);	ff("EFDABC", E, F, D, A, B, C, S);
			ff("AFEBDC", A, F, E, B, D, C, S);	ff("CFEADB", C, F, E, A, D, B, S);	ff("EFDACB", E, F, D, A, C, B, S);
			ff("AFECBD", A, F, E, C, B, D, S);	ff("CFEBAD", C, F, E, B, A, D, S);	ff("EFDBAC", E, F, D, B, A, C, S);
			ff("AFECDB", A, F, E, C, D, B, S);	ff("CFEBDA", C, F, E, B, D, A, S);	ff("EFDBCA", E, F, D, B, C, A, S);
			ff("AFEDBC", A, F, E, D, B, C, S);	ff("CFEDAB", C, F, E, D, A, B, S);	ff("EFDCAB", E, F, D, C, A, B, S);
			ff("AFEDCB", A, F, E, D, C, B, S);	ff("CFEDBA", C, F, E, D, B, A, S);	ff("EFDCBA", E, F, D, C, B, A, S);

			ff("BACDEF", B, A, C, D, E, F, S);	ff("DABCEF", D, A, B, C, E, F, S);	ff("FABCDE", F, A, B, C, D, E, S);
			ff("BACDFE", B, A, C, D, F, E, S);	ff("DABCFE", D, A, B, C, F, E, S);	ff("FABCED", F, A, B, C, E, D, S);
			ff("BACEDF", B, A, C, E, D, F, S);	ff("DABECF", D, A, B, E, C, F, S);	ff("FABDCE", F, A, B, D, C, E, S);
			ff("BACEFD", B, A, C, E, F, D, S);	ff("DABEFC", D, A, B, E, F, C, S);	ff("FABDEC", F, A, B, D, E, C, S);
			ff("BACFDE", B, A, C, F, D, E, S);	ff("DABFCE", D, A, B, F, C, E, S);	ff("FABECD", F, A, B, E, C, D, S);
			ff("BACFED", B, A, C, F, E, D, S);	ff("DABFEC", D, A, B, F, E, C, S);	ff("FABEDC", F, A, B, E, D, C, S);
			ff("BADCEF", B, A, D, C, E, F, S);	ff("DACBEF", D, A, C, B, E, F, S);	ff("FACBDE", F, A, C, B, D, E, S);
			ff("BADCFE", B, A, D, C, F, E, S);	ff("DACBFE", D, A, C, B, F, E, S);	ff("FACBED", F, A, C, B, E, D, S);
			ff("BADECF", B, A, D, E, C, F, S);	ff("DACEBF", D, A, C, E, B, F, S);	ff("FACDBE", F, A, C, D, B, E, S);
			ff("BADEFC", B, A, D, E, F, C, S);	ff("DACEFB", D, A, C, E, F, B, S);	ff("FACDEB", F, A, C, D, E, B, S);
			ff("BADFCE", B, A, D, F, C, E, S);	ff("DACFBE", D, A, C, F, B, E, S);	ff("FACEBD", F, A, C, E, B, D, S);
			ff("BADFEC", B, A, D, F, E, C, S);	ff("DACFEB", D, A, C, F, E, B, S);	ff("FACEDB", F, A, C, E, D, B, S);
			ff("BAECDF", B, A, E, C, D, F, S);	ff("DAEBCF", D, A, E, B, C, F, S);	ff("FADBCE", F, A, D, B, C, E, S);
			ff("BAECFD", B, A, E, C, F, D, S);	ff("DAEBFC", D, A, E, B, F, C, S);	ff("FADBEC", F, A, D, B, E, C, S);
			ff("BAEDCF", B, A, E, D, C, F, S);	ff("DAECBF", D, A, E, C, B, F, S);	ff("FADCBE", F, A, D, C, B, E, S);
			ff("BAEDFC", B, A, E, D, F, C, S);	ff("DAECFB", D, A, E, C, F, B, S);	ff("FADCEB", F, A, D, C, E, B, S);
			ff("BAEFCD", B, A, E, F, C, D, S);	ff("DAEFBC", D, A, E, F, B, C, S);	ff("FADEBC", F, A, D, E, B, C, S);
			ff("BAEFDC", B, A, E, F, D, C, S);	ff("DAEFCB", D, A, E, F, C, B, S);	ff("FADECB", F, A, D, E, C, B, S);
			ff("BAFCDE", B, A, F, C, D, E, S);	ff("DAFBCE", D, A, F, B, C, E, S);	ff("FAEBCD", F, A, E, B, C, D, S);
			ff("BAFCED", B, A, F, C, E, D, S);	ff("DAFBEC", D, A, F, B, E, C, S);	ff("FAEBDC", F, A, E, B, D, C, S);
			ff("BAFDCE", B, A, F, D, C, E, S);	ff("DAFCBE", D, A, F, C, B, E, S);	ff("FAECBD", F, A, E, C, B, D, S);
			ff("BAFDEC", B, A, F, D, E, C, S);	ff("DAFCEB", D, A, F, C, E, B, S);	ff("FAECDB", F, A, E, C, D, B, S);
			ff("BAFECD", B, A, F, E, C, D, S);	ff("DAFEBC", D, A, F, E, B, C, S);	ff("FAEDBC", F, A, E, D, B, C, S);
			ff("BAFEDC", B, A, F, E, D, C, S);	ff("DAFECB", D, A, F, E, C, B, S);	ff("FAEDCB", F, A, E, D, C, B, S);
			ff("BCADEF", B, C, A, D, E, F, S);	ff("DBACEF", D, B, A, C, E, F, S);	ff("FBACDE", F, B, A, C, D, E, S);
			ff("BCADFE", B, C, A, D, F, E, S);	ff("DBACFE", D, B, A, C, F, E, S);	ff("FBACED", F, B, A, C, E, D, S);
			ff("BCAEDF", B, C, A, E, D, F, S);	ff("DBAECF", D, B, A, E, C, F, S);	ff("FBADCE", F, B, A, D, C, E, S);
			ff("BCAEFD", B, C, A, E, F, D, S);	ff("DBAEFC", D, B, A, E, F, C, S);	ff("FBADEC", F, B, A, D, E, C, S);
			ff("BCAFDE", B, C, A, F, D, E, S);	ff("DBAFCE", D, B, A, F, C, E, S);	ff("FBAECD", F, B, A, E, C, D, S);
			ff("BCAFED", B, C, A, F, E, D, S);	ff("DBAFEC", D, B, A, F, E, C, S);	ff("FBAEDC", F, B, A, E, D, C, S);
			ff("BCDAEF", B, C, D, A, E, F, S);	ff("DBCAEF", D, B, C, A, E, F, S);	ff("FBCADE", F, B, C, A, D, E, S);
			ff("BCDAFE", B, C, D, A, F, E, S);	ff("DBCAFE", D, B, C, A, F, E, S);	ff("FBCAED", F, B, C, A, E, D, S);
			ff("BCDEAF", B, C, D, E, A, F, S);	ff("DBCEAF", D, B, C, E, A, F, S);	ff("FBCDAE", F, B, C, D, A, E, S);
			ff("BCDEFA", B, C, D, E, F, A, S);	ff("DBCEFA", D, B, C, E, F, A, S);	ff("FBCDEA", F, B, C, D, E, A, S);
			ff("BCDFAE", B, C, D, F, A, E, S);	ff("DBCFAE", D, B, C, F, A, E, S);	ff("FBCEAD", F, B, C, E, A, D, S);
			ff("BCDFEA", B, C, D, F, E, A, S);	ff("DBCFEA", D, B, C, F, E, A, S);	ff("FBCEDA", F, B, C, E, D, A, S);
			ff("BCEADF", B, C, E, A, D, F, S);	ff("DBEACF", D, B, E, A, C, F, S);	ff("FBDACE", F, B, D, A, C, E, S);
			ff("BCEAFD", B, C, E, A, F, D, S);	ff("DBEAFC", D, B, E, A, F, C, S);	ff("FBDAEC", F, B, D, A, E, C, S);
			ff("BCEDAF", B, C, E, D, A, F, S);	ff("DBECAF", D, B, E, C, A, F, S);	ff("FBDCAE", F, B, D, C, A, E, S);
			ff("BCEDFA", B, C, E, D, F, A, S);	ff("DBECFA", D, B, E, C, F, A, S);	ff("FBDCEA", F, B, D, C, E, A, S);
			ff("BCEFAD", B, C, E, F, A, D, S);	ff("DBEFAC", D, B, E, F, A, C, S);	ff("FBDEAC", F, B, D, E, A, C, S);
			ff("BCEFDA", B, C, E, F, D, A, S);	ff("DBEFCA", D, B, E, F, C, A, S);	ff("FBDECA", F, B, D, E, C, A, S);
			ff("BCFADE", B, C, F, A, D, E, S);	ff("DBFACE", D, B, F, A, C, E, S);	ff("FBEACD", F, B, E, A, C, D, S);
			ff("BCFAED", B, C, F, A, E, D, S);	ff("DBFAEC", D, B, F, A, E, C, S);	ff("FBEADC", F, B, E, A, D, C, S);
			ff("BCFDAE", B, C, F, D, A, E, S);	ff("DBFCAE", D, B, F, C, A, E, S);	ff("FBECAD", F, B, E, C, A, D, S);
			ff("BCFDEA", B, C, F, D, E, A, S);	ff("DBFCEA", D, B, F, C, E, A, S);	ff("FBECDA", F, B, E, C, D, A, S);
			ff("BCFEAD", B, C, F, E, A, D, S);	ff("DBFEAC", D, B, F, E, A, C, S);	ff("FBEDAC", F, B, E, D, A, C, S);
			ff("BCFEDA", B, C, F, E, D, A, S);	ff("DBFECA", D, B, F, E, C, A, S);	ff("FBEDCA", F, B, E, D, C, A, S);
			ff("BDACEF", B, D, A, C, E, F, S);	ff("DCABEF", D, C, A, B, E, F, S);	ff("FCABDE", F, C, A, B, D, E, S);
			ff("BDACFE", B, D, A, C, F, E, S);	ff("DCABFE", D, C, A, B, F, E, S);	ff("FCABED", F, C, A, B, E, D, S);
			ff("BDAECF", B, D, A, E, C, F, S);	ff("DCAEBF", D, C, A, E, B, F, S);	ff("FCADBE", F, C, A, D, B, E, S);
			ff("BDAEFC", B, D, A, E, F, C, S);	ff("DCAEFB", D, C, A, E, F, B, S);	ff("FCADEB", F, C, A, D, E, B, S);
			ff("BDAFCE", B, D, A, F, C, E, S);	ff("DCAFBE", D, C, A, F, B, E, S);	ff("FCAEBD", F, C, A, E, B, D, S);
			ff("BDAFEC", B, D, A, F, E, C, S);	ff("DCAFEB", D, C, A, F, E, B, S);	ff("FCAEDB", F, C, A, E, D, B, S);
			ff("BDCAEF", B, D, C, A, E, F, S);	ff("DCBAEF", D, C, B, A, E, F, S);	ff("FCBADE", F, C, B, A, D, E, S);
			ff("BDCAFE", B, D, C, A, F, E, S);	ff("DCBAFE", D, C, B, A, F, E, S);	ff("FCBAED", F, C, B, A, E, D, S);
			ff("BDCEAF", B, D, C, E, A, F, S);	ff("DCBEAF", D, C, B, E, A, F, S);	ff("FCBDAE", F, C, B, D, A, E, S);
			ff("BDCEFA", B, D, C, E, F, A, S);	ff("DCBEFA", D, C, B, E, F, A, S);	ff("FCBDEA", F, C, B, D, E, A, S);
			ff("BDCFAE", B, D, C, F, A, E, S);	ff("DCBFAE", D, C, B, F, A, E, S);	ff("FCBEAD", F, C, B, E, A, D, S);
			ff("BDCFEA", B, D, C, F, E, A, S);	ff("DCBFEA", D, C, B, F, E, A, S);	ff("FCBEDA", F, C, B, E, D, A, S);
			ff("BDEACF", B, D, E, A, C, F, S);	ff("DCEABF", D, C, E, A, B, F, S);	ff("FCDABE", F, C, D, A, B, E, S);
			ff("BDEAFC", B, D, E, A, F, C, S);	ff("DCEAFB", D, C, E, A, F, B, S);	ff("FCDAEB", F, C, D, A, E, B, S);
			ff("BDECAF", B, D, E, C, A, F, S);	ff("DCEBAF", D, C, E, B, A, F, S);	ff("FCDBAE", F, C, D, B, A, E, S);
			ff("BDECFA", B, D, E, C, F, A, S);	ff("DCEBFA", D, C, E, B, F, A, S);	ff("FCDBEA", F, C, D, B, E, A, S);
			ff("BDEFAC", B, D, E, F, A, C, S);	ff("DCEFAB", D, C, E, F, A, B, S);	ff("FCDEAB", F, C, D, E, A, B, S);
			ff("BDEFCA", B, D, E, F, C, A, S);	ff("DCEFBA", D, C, E, F, B, A, S);	ff("FCDEBA", F, C, D, E, B, A, S);
			ff("BDFACE", B, D, F, A, C, E, S);	ff("DCFABE", D, C, F, A, B, E, S);	ff("FCEABD", F, C, E, A, B, D, S);
			ff("BDFAEC", B, D, F, A, E, C, S);	ff("DCFAEB", D, C, F, A, E, B, S);	ff("FCEADB", F, C, E, A, D, B, S);
			ff("BDFCAE", B, D, F, C, A, E, S);	ff("DCFBAE", D, C, F, B, A, E, S);	ff("FCEBAD", F, C, E, B, A, D, S);
			ff("BDFCEA", B, D, F, C, E, A, S);	ff("DCFBEA", D, C, F, B, E, A, S);	ff("FCEBDA", F, C, E, B, D, A, S);
			ff("BDFEAC", B, D, F, E, A, C, S);	ff("DCFEAB", D, C, F, E, A, B, S);	ff("FCEDAB", F, C, E, D, A, B, S);
			ff("BDFECA", B, D, F, E, C, A, S);	ff("DCFEBA", D, C, F, E, B, A, S);	ff("FCEDBA", F, C, E, D, B, A, S);
			ff("BEACDF", B, E, A, C, D, F, S);	ff("DEABCF", D, E, A, B, C, F, S);	ff("FDABCE", F, D, A, B, C, E, S);
			ff("BEACFD", B, E, A, C, F, D, S);	ff("DEABFC", D, E, A, B, F, C, S);	ff("FDABEC", F, D, A, B, E, C, S);
			ff("BEADCF", B, E, A, D, C, F, S);	ff("DEACBF", D, E, A, C, B, F, S);	ff("FDACBE", F, D, A, C, B, E, S);
			ff("BEADFC", B, E, A, D, F, C, S);	ff("DEACFB", D, E, A, C, F, B, S);	ff("FDACEB", F, D, A, C, E, B, S);
			ff("BEAFCD", B, E, A, F, C, D, S);	ff("DEAFBC", D, E, A, F, B, C, S);	ff("FDAEBC", F, D, A, E, B, C, S);
			ff("BEAFDC", B, E, A, F, D, C, S);	ff("DEAFCB", D, E, A, F, C, B, S);	ff("FDAECB", F, D, A, E, C, B, S);
			ff("BECADF", B, E, C, A, D, F, S);	ff("DEBACF", D, E, B, A, C, F, S);	ff("FDBACE", F, D, B, A, C, E, S);
			ff("BECAFD", B, E, C, A, F, D, S);	ff("DEBAFC", D, E, B, A, F, C, S);	ff("FDBAEC", F, D, B, A, E, C, S);
			ff("BECDAF", B, E, C, D, A, F, S);	ff("DEBCAF", D, E, B, C, A, F, S);	ff("FDBCAE", F, D, B, C, A, E, S);
			ff("BECDFA", B, E, C, D, F, A, S);	ff("DEBCFA", D, E, B, C, F, A, S);	ff("FDBCEA", F, D, B, C, E, A, S);
			ff("BECFAD", B, E, C, F, A, D, S);	ff("DEBFAC", D, E, B, F, A, C, S);	ff("FDBEAC", F, D, B, E, A, C, S);
			ff("BECFDA", B, E, C, F, D, A, S);	ff("DEBFCA", D, E, B, F, C, A, S);	ff("FDBECA", F, D, B, E, C, A, S);
			ff("BEDACF", B, E, D, A, C, F, S);	ff("DECABF", D, E, C, A, B, F, S);	ff("FDCABE", F, D, C, A, B, E, S);
			ff("BEDAFC", B, E, D, A, F, C, S);	ff("DECAFB", D, E, C, A, F, B, S);	ff("FDCAEB", F, D, C, A, E, B, S);
			ff("BEDCAF", B, E, D, C, A, F, S);	ff("DECBAF", D, E, C, B, A, F, S);	ff("FDCBAE", F, D, C, B, A, E, S);
			ff("BEDCFA", B, E, D, C, F, A, S);	ff("DECBFA", D, E, C, B, F, A, S);	ff("FDCBEA", F, D, C, B, E, A, S);
			ff("BEDFAC", B, E, D, F, A, C, S);	ff("DECFAB", D, E, C, F, A, B, S);	ff("FDCEAB", F, D, C, E, A, B, S);
			ff("BEDFCA", B, E, D, F, C, A, S);	ff("DECFBA", D, E, C, F, B, A, S);	ff("FDCEBA", F, D, C, E, B, A, S);
			ff("BEFACD", B, E, F, A, C, D, S);	ff("DEFABC", D, E, F, A, B, C, S);	ff("FDEABC", F, D, E, A, B, C, S);
			ff("BEFADC", B, E, F, A, D, C, S);	ff("DEFACB", D, E, F, A, C, B, S);	ff("FDEACB", F, D, E, A, C, B, S);
			ff("BEFCAD", B, E, F, C, A, D, S);	ff("DEFBAC", D, E, F, B, A, C, S);	ff("FDEBAC", F, D, E, B, A, C, S);
			ff("BEFCDA", B, E, F, C, D, A, S);	ff("DEFBCA", D, E, F, B, C, A, S);	ff("FDEBCA", F, D, E, B, C, A, S);
			ff("BEFDAC", B, E, F, D, A, C, S);	ff("DEFCAB", D, E, F, C, A, B, S);	ff("FDECAB", F, D, E, C, A, B, S);
			ff("BEFDCA", B, E, F, D, C, A, S);	ff("DEFCBA", D, E, F, C, B, A, S);	ff("FDECBA", F, D, E, C, B, A, S);
			ff("BFACDE", B, F, A, C, D, E, S);	ff("DFABCE", D, F, A, B, C, E, S);	ff("FEABCD", F, E, A, B, C, D, S);
			ff("BFACED", B, F, A, C, E, D, S);	ff("DFABEC", D, F, A, B, E, C, S);	ff("FEABDC", F, E, A, B, D, C, S);
			ff("BFADCE", B, F, A, D, C, E, S);	ff("DFACBE", D, F, A, C, B, E, S);	ff("FEACBD", F, E, A, C, B, D, S);
			ff("BFADEC", B, F, A, D, E, C, S);	ff("DFACEB", D, F, A, C, E, B, S);	ff("FEACDB", F, E, A, C, D, B, S);
			ff("BFAECD", B, F, A, E, C, D, S);	ff("DFAEBC", D, F, A, E, B, C, S);	ff("FEADBC", F, E, A, D, B, C, S);
			ff("BFAEDC", B, F, A, E, D, C, S);	ff("DFAECB", D, F, A, E, C, B, S);	ff("FEADCB", F, E, A, D, C, B, S);
			ff("BFCADE", B, F, C, A, D, E, S);	ff("DFBACE", D, F, B, A, C, E, S);	ff("FEBACD", F, E, B, A, C, D, S);
			ff("BFCAED", B, F, C, A, E, D, S);	ff("DFBAEC", D, F, B, A, E, C, S);	ff("FEBADC", F, E, B, A, D, C, S);
			ff("BFCDAE", B, F, C, D, A, E, S);	ff("DFBCAE", D, F, B, C, A, E, S);	ff("FEBCAD", F, E, B, C, A, D, S);
			ff("BFCDEA", B, F, C, D, E, A, S);	ff("DFBCEA", D, F, B, C, E, A, S);	ff("FEBCDA", F, E, B, C, D, A, S);
			ff("BFCEAD", B, F, C, E, A, D, S);	ff("DFBEAC", D, F, B, E, A, C, S);	ff("FEBDAC", F, E, B, D, A, C, S);
			ff("BFCEDA", B, F, C, E, D, A, S);	ff("DFBECA", D, F, B, E, C, A, S);	ff("FEBDCA", F, E, B, D, C, A, S);
			ff("BFDACE", B, F, D, A, C, E, S);	ff("DFCABE", D, F, C, A, B, E, S);	ff("FECABD", F, E, C, A, B, D, S);
			ff("BFDAEC", B, F, D, A, E, C, S);	ff("DFCAEB", D, F, C, A, E, B, S);	ff("FECADB", F, E, C, A, D, B, S);
			ff("BFDCAE", B, F, D, C, A, E, S);	ff("DFCBAE", D, F, C, B, A, E, S);	ff("FECBAD", F, E, C, B, A, D, S);
			ff("BFDCEA", B, F, D, C, E, A, S);	ff("DFCBEA", D, F, C, B, E, A, S);	ff("FECBDA", F, E, C, B, D, A, S);
			ff("BFDEAC", B, F, D, E, A, C, S);	ff("DFCEAB", D, F, C, E, A, B, S);	ff("FECDAB", F, E, C, D, A, B, S);
			ff("BFDECA", B, F, D, E, C, A, S);	ff("DFCEBA", D, F, C, E, B, A, S);	ff("FECDBA", F, E, C, D, B, A, S);
			ff("BFEACD", B, F, E, A, C, D, S);	ff("DFEABC", D, F, E, A, B, C, S);	ff("FEDABC", F, E, D, A, B, C, S);
			ff("BFEADC", B, F, E, A, D, C, S);	ff("DFEACB", D, F, E, A, C, B, S);	ff("FEDACB", F, E, D, A, C, B, S);
			ff("BFECAD", B, F, E, C, A, D, S);	ff("DFEBAC", D, F, E, B, A, C, S);	ff("FEDBAC", F, E, D, B, A, C, S);
			ff("BFECDA", B, F, E, C, D, A, S);	ff("DFEBCA", D, F, E, B, C, A, S);	ff("FEDBCA", F, E, D, B, C, A, S);
			ff("BFEDAC", B, F, E, D, A, C, S);	ff("DFECAB", D, F, E, C, A, B, S);	ff("FEDCAB", F, E, D, C, A, B, S);
			ff("BFEDCA", B, F, E, D, C, A, S);	ff("DFECBA", D, F, E, C, B, A, S);	ff("FEDCBA", F, E, D, C, B, A, S);
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

