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
			auto const A = indexes[0];

			auto ff = [&](std::string_view txt, std::string_view a){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, keySub);

				func(key);
			};

			// old style not supports txt
			ff("X", A);
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
			auto const A = indexes[0];

			auto ff = [&](std::string_view txt, std::string_view a){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, keySub);

				func(key);
			};

			ff("A", A);
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
			auto const A = indexes[0];
			auto const B = indexes[1];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, keySub);

				func(key);
			};

			ff("AB", A, B);
			ff("BA", B, A);
		}
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
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, keySub);

				func(key);
			};

			ff("ABC", A, B, C);
			ff("ACB", A, C, B);
			ff("BAC", B, A, C);
			ff("BCA", B, C, A);
			ff("CAB", C, A, B);
			ff("CBA", C, B, A);

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
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];

			auto ff = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, keySub);

				func(key);
			};

			ff("ABCD", A, B, C, D);
			ff("ABDC", A, B, D, C);
			ff("ACBD", A, C, B, D);
			ff("ACDB", A, C, D, B);
			ff("ADBC", A, D, B, C);
			ff("ADCB", A, D, C, B);
			ff("BACD", B, A, C, D);
			ff("BADC", B, A, D, C);
			ff("BCAD", B, C, A, D);
			ff("BCDA", B, C, D, A);
			ff("BDAC", B, D, A, C);
			ff("BDCA", B, D, C, A);
			ff("CABD", C, A, B, D);
			ff("CADB", C, A, D, B);
			ff("CBAD", C, B, A, D);
			ff("CBDA", C, B, D, A);
			ff("CDAB", C, D, A, B);
			ff("CDBA", C, D, B, A);
			ff("DABC", D, A, B, C);
			ff("DACB", D, A, C, B);
			ff("DBAC", D, B, A, C);
			ff("DBCA", D, B, C, A);
			ff("DCAB", D, C, A, B);
			ff("DCBA", D, C, B, A);
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
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];
			auto const E = indexes[4];

			auto ff = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, keySub);

				func(key);
			};

			// All 120 permutations here:
			// thanks chatgpt

			ff("ABCDE", A, B, C, D, E);
			ff("ABCED", A, B, C, E, D);
			ff("ABDCE", A, B, D, C, E);
			ff("ABDEC", A, B, D, E, C);
			ff("ABECD", A, B, E, C, D);
			ff("ABEDC", A, B, E, D, C);
			ff("ACBDE", A, C, B, D, E);
			ff("ACBED", A, C, B, E, D);
			ff("ACDBE", A, C, D, B, E);
			ff("ACDEB", A, C, D, E, B);
			ff("ACEBD", A, C, E, B, D);
			ff("ACEDC", A, C, E, D, C);
			ff("ADBCE", A, D, B, C, E);
			ff("ADBEC", A, D, B, E, C);
			ff("ADCBE", A, D, C, B, E);
			ff("ADCEB", A, D, C, E, B);
			ff("ADEBC", A, D, E, B, C);
			ff("ADECB", A, D, E, C, B);
			ff("AEBCD", A, E, B, C, D);
			ff("AEBDC", A, E, B, D, C);
			ff("AECBD", A, E, C, B, D);
			ff("AECDB", A, E, C, D, B);
			ff("AEDBC", A, E, D, B, C);
			ff("AEDCB", A, E, D, C, B);
			ff("BACDE", B, A, C, D, E);
			ff("BACED", B, A, C, E, D);
			ff("BADCE", B, A, D, C, E);
			ff("BADEC", B, A, D, E, C);
			ff("BAECD", B, A, E, C, D);
			ff("BAEDC", B, A, E, D, C);
			ff("BCADE", B, C, A, D, E);
			ff("BCAED", B, C, A, E, D);
			ff("BCDAE", B, C, D, A, E);
			ff("BCDEA", B, C, D, E, A);
			ff("BCEAD", B, C, E, A, D);
			ff("BCEDA", B, C, E, D, A);
			ff("BDACE", B, D, A, C, E);
			ff("BDAEC", B, D, A, E, C);
			ff("BDCAE", B, D, C, A, E);
			ff("BDCEA", B, D, C, E, A);
			ff("BDEAC", B, D, E, A, C);
			ff("BDECA", B, D, E, C, A);
			ff("BEACD", B, E, A, C, D);
			ff("BEADC", B, E, A, D, C);
			ff("BECAD", B, E, C, A, D);
			ff("BECDA", B, E, C, D, A);
			ff("BEDAC", B, E, D, A, C);
			ff("BEDCA", B, E, D, C, A);
			ff("CABDE", C, A, B, D, E);
			ff("CABED", C, A, B, E, D);
			ff("CADBE", C, A, D, B, E);
			ff("CADEB", C, A, D, E, B);
			ff("CAEBD", C, A, E, B, D);
			ff("CAEDB", C, A, E, D, B);
			ff("CBADE", C, B, A, D, E);
			ff("CBAED", C, B, A, E, D);
			ff("CBDAE", C, B, D, A, E);
			ff("CBDEA", C, B, D, E, A);
			ff("CBEAD", C, B, E, A, D);
			ff("CBEDA", C, B, E, D, A);
			ff("CDABE", C, D, A, B, E);
			ff("CDAEB", C, D, A, E, B);
			ff("CDBAE", C, D, B, A, E);
			ff("CDBEA", C, D, B, E, A);
			ff("CDEAB", C, D, E, A, B);
			ff("CDEBA", C, D, E, B, A);
			ff("CEABD", C, E, A, B, D);
			ff("CEADB", C, E, A, D, B);
			ff("CEBAD", C, E, B, A, D);
			ff("CEBDA", C, E, B, D, A);
			ff("CEDAB", C, E, D, A, B);
			ff("CEDBA", C, E, D, B, A);
			ff("DABCE", D, A, B, C, E);
			ff("DABEC", D, A, B, E, C);
			ff("DACBE", D, A, C, B, E);
			ff("DACEB", D, A, C, E, B);
			ff("DAEBC", D, A, E, B, C);
			ff("DAECB", D, A, E, C, B);
			ff("DBACE", D, B, A, C, E);
			ff("DBAEC", D, B, A, E, C);
			ff("DBCAE", D, B, C, A, E);
			ff("DBCEA", D, B, C, E, A);
			ff("DBEAC", D, B, E, A, C);
			ff("DBECA", D, B, E, C, A);
			ff("DCABE", D, C, A, B, E);
			ff("DCAEB", D, C, A, E, B);
			ff("DCBAE", D, C, B, A, E);
			ff("DCBEA", D, C, B, E, A);
			ff("DCEAB", D, C, E, A, B);
			ff("DCEBA", D, C, E, B, A);
			ff("DEABC", D, E, A, B, C);
			ff("DEACB", D, E, A, C, B);
			ff("DEBAC", D, E, B, A, C);
			ff("DEBCA", D, E, B, C, A);
			ff("DECAB", D, E, C, A, B);
			ff("DECBA", D, E, C, B, A);
			ff("EABCD", E, A, B, C, D);
			ff("EABDC", E, A, B, D, C);
			ff("EACBD", E, A, C, B, D);
			ff("EACDB", E, A, C, D, B);
			ff("EADBC", E, A, D, B, C);
			ff("EADCB", E, A, D, C, B);
			ff("EBACD", E, B, A, C, D);
			ff("EBADC", E, B, A, D, C);
			ff("EBCAD", E, B, C, A, D);
			ff("EBCDA", E, B, C, D, A);
			ff("EBDAC", E, B, D, A, C);
			ff("EBDCA", E, B, D, C, A);
			ff("ECABD", E, C, A, B, D);
			ff("ECADB", E, C, A, D, B);
			ff("ECBAD", E, C, B, A, D);
			ff("ECBDA", E, C, B, D, A);
			ff("ECDAB", E, C, D, A, B);
			ff("ECDBA", E, C, D, B, A);
			ff("EDABC", E, D, A, B, C);
			ff("EDACB", E, D, A, C, B);
			ff("EDBAC", E, D, B, A, C);
			ff("EDBCA", E, D, B, C, A);
			ff("EDCAB", E, D, C, A, B);
			ff("EDCBA", E, D, C, B, A);
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
			auto const A = indexes[0];
			auto const B = indexes[1];
			auto const C = indexes[2];
			auto const D = indexes[3];
			auto const E = indexes[4];
			auto const F = indexes[5];

			auto ff = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e, std::string_view f){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, f, keySub);

				func(key);
			};

			// All 720 permutations here:
			// thanks wolfram alpha
			// this is still acceptable, sinse XNDEL can do 32K inserts.

			ff("ABCDEF", A, B, C, D, E, F);	ff("CABDEF", C, A, B, D, E, F);	ff("EABCDF", E, A, B, C, D, F);
			ff("ABCDFE", A, B, C, D, F, E);	ff("CABDFE", C, A, B, D, F, E);	ff("EABCFD", E, A, B, C, F, D);
			ff("ABCEDF", A, B, C, E, D, F);	ff("CABEDF", C, A, B, E, D, F);	ff("EABDCF", E, A, B, D, C, F);
			ff("ABCEFD", A, B, C, E, F, D);	ff("CABEFD", C, A, B, E, F, D);	ff("EABDFC", E, A, B, D, F, C);
			ff("ABCFDE", A, B, C, F, D, E);	ff("CABFDE", C, A, B, F, D, E);	ff("EABFCD", E, A, B, F, C, D);
			ff("ABCFED", A, B, C, F, E, D);	ff("CABFED", C, A, B, F, E, D);	ff("EABFDC", E, A, B, F, D, C);
			ff("ABDCEF", A, B, D, C, E, F);	ff("CADBEF", C, A, D, B, E, F);	ff("EACBDF", E, A, C, B, D, F);
			ff("ABDCFE", A, B, D, C, F, E);	ff("CADBFE", C, A, D, B, F, E);	ff("EACBFD", E, A, C, B, F, D);
			ff("ABDECF", A, B, D, E, C, F);	ff("CADEBF", C, A, D, E, B, F);	ff("EACDBF", E, A, C, D, B, F);
			ff("ABDEFC", A, B, D, E, F, C);	ff("CADEFB", C, A, D, E, F, B);	ff("EACDFB", E, A, C, D, F, B);
			ff("ABDFCE", A, B, D, F, C, E);	ff("CADFBE", C, A, D, F, B, E);	ff("EACFBD", E, A, C, F, B, D);
			ff("ABDFEC", A, B, D, F, E, C);	ff("CADFEB", C, A, D, F, E, B);	ff("EACFDB", E, A, C, F, D, B);
			ff("ABECDF", A, B, E, C, D, F);	ff("CAEBDF", C, A, E, B, D, F);	ff("EADBCF", E, A, D, B, C, F);
			ff("ABECFD", A, B, E, C, F, D);	ff("CAEBFD", C, A, E, B, F, D);	ff("EADBFC", E, A, D, B, F, C);
			ff("ABEDCF", A, B, E, D, C, F);	ff("CAEDBF", C, A, E, D, B, F);	ff("EADCBF", E, A, D, C, B, F);
			ff("ABEDFC", A, B, E, D, F, C);	ff("CAEDFB", C, A, E, D, F, B);	ff("EADCFB", E, A, D, C, F, B);
			ff("ABEFCD", A, B, E, F, C, D);	ff("CAEFBD", C, A, E, F, B, D);	ff("EADFBC", E, A, D, F, B, C);
			ff("ABEFDC", A, B, E, F, D, C);	ff("CAEFDB", C, A, E, F, D, B);	ff("EADFCB", E, A, D, F, C, B);
			ff("ABFCDE", A, B, F, C, D, E);	ff("CAFBDE", C, A, F, B, D, E);	ff("EAFBCD", E, A, F, B, C, D);
			ff("ABFCED", A, B, F, C, E, D);	ff("CAFBED", C, A, F, B, E, D);	ff("EAFBDC", E, A, F, B, D, C);
			ff("ABFDCE", A, B, F, D, C, E);	ff("CAFDBE", C, A, F, D, B, E);	ff("EAFCBD", E, A, F, C, B, D);
			ff("ABFDEC", A, B, F, D, E, C);	ff("CAFDEB", C, A, F, D, E, B);	ff("EAFCDB", E, A, F, C, D, B);
			ff("ABFECD", A, B, F, E, C, D);	ff("CAFEBD", C, A, F, E, B, D);	ff("EAFDBC", E, A, F, D, B, C);
			ff("ABFEDC", A, B, F, E, D, C);	ff("CAFEDB", C, A, F, E, D, B);	ff("EAFDCB", E, A, F, D, C, B);
			ff("ACBDEF", A, C, B, D, E, F);	ff("CBADEF", C, B, A, D, E, F);	ff("EBACDF", E, B, A, C, D, F);
			ff("ACBDFE", A, C, B, D, F, E);	ff("CBADFE", C, B, A, D, F, E);	ff("EBACFD", E, B, A, C, F, D);
			ff("ACBEDF", A, C, B, E, D, F);	ff("CBAEDF", C, B, A, E, D, F);	ff("EBADCF", E, B, A, D, C, F);
			ff("ACBEFD", A, C, B, E, F, D);	ff("CBAEFD", C, B, A, E, F, D);	ff("EBADFC", E, B, A, D, F, C);
			ff("ACBFDE", A, C, B, F, D, E);	ff("CBAFDE", C, B, A, F, D, E);	ff("EBAFCD", E, B, A, F, C, D);
			ff("ACBFED", A, C, B, F, E, D);	ff("CBAFED", C, B, A, F, E, D);	ff("EBAFDC", E, B, A, F, D, C);
			ff("ACDBEF", A, C, D, B, E, F);	ff("CBDAEF", C, B, D, A, E, F);	ff("EBCADF", E, B, C, A, D, F);
			ff("ACDBFE", A, C, D, B, F, E);	ff("CBDAFE", C, B, D, A, F, E);	ff("EBCAFD", E, B, C, A, F, D);
			ff("ACDEBF", A, C, D, E, B, F);	ff("CBDEAF", C, B, D, E, A, F);	ff("EBCDAF", E, B, C, D, A, F);
			ff("ACDEFB", A, C, D, E, F, B);	ff("CBDEFA", C, B, D, E, F, A);	ff("EBCDFA", E, B, C, D, F, A);
			ff("ACDFBE", A, C, D, F, B, E);	ff("CBDFAE", C, B, D, F, A, E);	ff("EBCFAD", E, B, C, F, A, D);
			ff("ACDFEB", A, C, D, F, E, B);	ff("CBDFEA", C, B, D, F, E, A);	ff("EBCFDA", E, B, C, F, D, A);
			ff("ACEBDF", A, C, E, B, D, F);	ff("CBEADF", C, B, E, A, D, F);	ff("EBDACF", E, B, D, A, C, F);
			ff("ACEBFD", A, C, E, B, F, D);	ff("CBEAFD", C, B, E, A, F, D);	ff("EBDAFC", E, B, D, A, F, C);
			ff("ACEDBF", A, C, E, D, B, F);	ff("CBEDAF", C, B, E, D, A, F);	ff("EBDCAF", E, B, D, C, A, F);
			ff("ACEDFB", A, C, E, D, F, B);	ff("CBEDFA", C, B, E, D, F, A);	ff("EBDCFA", E, B, D, C, F, A);
			ff("ACEFBD", A, C, E, F, B, D);	ff("CBEFAD", C, B, E, F, A, D);	ff("EBDFAC", E, B, D, F, A, C);
			ff("ACEFDB", A, C, E, F, D, B);	ff("CBEFDA", C, B, E, F, D, A);	ff("EBDFCA", E, B, D, F, C, A);
			ff("ACFBDE", A, C, F, B, D, E);	ff("CBFADE", C, B, F, A, D, E);	ff("EBFACD", E, B, F, A, C, D);
			ff("ACFBED", A, C, F, B, E, D);	ff("CBFAED", C, B, F, A, E, D);	ff("EBFADC", E, B, F, A, D, C);
			ff("ACFDBE", A, C, F, D, B, E);	ff("CBFDAE", C, B, F, D, A, E);	ff("EBFCAD", E, B, F, C, A, D);
			ff("ACFDEB", A, C, F, D, E, B);	ff("CBFDEA", C, B, F, D, E, A);	ff("EBFCDA", E, B, F, C, D, A);
			ff("ACFEBD", A, C, F, E, B, D);	ff("CBFEAD", C, B, F, E, A, D);	ff("EBFDAC", E, B, F, D, A, C);
			ff("ACFEDB", A, C, F, E, D, B);	ff("CBFEDA", C, B, F, E, D, A);	ff("EBFDCA", E, B, F, D, C, A);
			ff("ADBCEF", A, D, B, C, E, F);	ff("CDABEF", C, D, A, B, E, F);	ff("ECABDF", E, C, A, B, D, F);
			ff("ADBCFE", A, D, B, C, F, E);	ff("CDABFE", C, D, A, B, F, E);	ff("ECABFD", E, C, A, B, F, D);
			ff("ADBECF", A, D, B, E, C, F);	ff("CDAEBF", C, D, A, E, B, F);	ff("ECADBF", E, C, A, D, B, F);
			ff("ADBEFC", A, D, B, E, F, C);	ff("CDAEFB", C, D, A, E, F, B);	ff("ECADFB", E, C, A, D, F, B);
			ff("ADBFCE", A, D, B, F, C, E);	ff("CDAFBE", C, D, A, F, B, E);	ff("ECAFBD", E, C, A, F, B, D);
			ff("ADBFEC", A, D, B, F, E, C);	ff("CDAFEB", C, D, A, F, E, B);	ff("ECAFDB", E, C, A, F, D, B);
			ff("ADCBEF", A, D, C, B, E, F);	ff("CDBAEF", C, D, B, A, E, F);	ff("ECBADF", E, C, B, A, D, F);
			ff("ADCBFE", A, D, C, B, F, E);	ff("CDBAFE", C, D, B, A, F, E);	ff("ECBAFD", E, C, B, A, F, D);
			ff("ADCEBF", A, D, C, E, B, F);	ff("CDBEAF", C, D, B, E, A, F);	ff("ECBDAF", E, C, B, D, A, F);
			ff("ADCEFB", A, D, C, E, F, B);	ff("CDBEFA", C, D, B, E, F, A);	ff("ECBDFA", E, C, B, D, F, A);
			ff("ADCFBE", A, D, C, F, B, E);	ff("CDBFAE", C, D, B, F, A, E);	ff("ECBFAD", E, C, B, F, A, D);
			ff("ADCFEB", A, D, C, F, E, B);	ff("CDBFEA", C, D, B, F, E, A);	ff("ECBFDA", E, C, B, F, D, A);
			ff("ADEBCF", A, D, E, B, C, F);	ff("CDEABF", C, D, E, A, B, F);	ff("ECDABF", E, C, D, A, B, F);
			ff("ADEBFC", A, D, E, B, F, C);	ff("CDEAFB", C, D, E, A, F, B);	ff("ECDAFB", E, C, D, A, F, B);
			ff("ADECBF", A, D, E, C, B, F);	ff("CDEBAF", C, D, E, B, A, F);	ff("ECDBAF", E, C, D, B, A, F);
			ff("ADECFB", A, D, E, C, F, B);	ff("CDEBFA", C, D, E, B, F, A);	ff("ECDBFA", E, C, D, B, F, A);
			ff("ADEFBC", A, D, E, F, B, C);	ff("CDEFAB", C, D, E, F, A, B);	ff("ECDFAB", E, C, D, F, A, B);
			ff("ADEFCB", A, D, E, F, C, B);	ff("CDEFBA", C, D, E, F, B, A);	ff("ECDFBA", E, C, D, F, B, A);
			ff("ADFBCE", A, D, F, B, C, E);	ff("CDFABE", C, D, F, A, B, E);	ff("ECFABD", E, C, F, A, B, D);
			ff("ADFBEC", A, D, F, B, E, C);	ff("CDFAEB", C, D, F, A, E, B);	ff("ECFADB", E, C, F, A, D, B);
			ff("ADFCBE", A, D, F, C, B, E);	ff("CDFBAE", C, D, F, B, A, E);	ff("ECFBAD", E, C, F, B, A, D);
			ff("ADFCEB", A, D, F, C, E, B);	ff("CDFBEA", C, D, F, B, E, A);	ff("ECFBDA", E, C, F, B, D, A);
			ff("ADFEBC", A, D, F, E, B, C);	ff("CDFEAB", C, D, F, E, A, B);	ff("ECFDAB", E, C, F, D, A, B);
			ff("ADFECB", A, D, F, E, C, B);	ff("CDFEBA", C, D, F, E, B, A);	ff("ECFDBA", E, C, F, D, B, A);
			ff("AEBCDF", A, E, B, C, D, F);	ff("CEABDF", C, E, A, B, D, F);	ff("EDABCF", E, D, A, B, C, F);
			ff("AEBCFD", A, E, B, C, F, D);	ff("CEABFD", C, E, A, B, F, D);	ff("EDABFC", E, D, A, B, F, C);
			ff("AEBDCF", A, E, B, D, C, F);	ff("CEADBF", C, E, A, D, B, F);	ff("EDACBF", E, D, A, C, B, F);
			ff("AEBDFC", A, E, B, D, F, C);	ff("CEADFB", C, E, A, D, F, B);	ff("EDACFB", E, D, A, C, F, B);
			ff("AEBFCD", A, E, B, F, C, D);	ff("CEAFBD", C, E, A, F, B, D);	ff("EDAFBC", E, D, A, F, B, C);
			ff("AEBFDC", A, E, B, F, D, C);	ff("CEAFDB", C, E, A, F, D, B);	ff("EDAFCB", E, D, A, F, C, B);
			ff("AECBDF", A, E, C, B, D, F);	ff("CEBADF", C, E, B, A, D, F);	ff("EDBACF", E, D, B, A, C, F);
			ff("AECBFD", A, E, C, B, F, D);	ff("CEBAFD", C, E, B, A, F, D);	ff("EDBAFC", E, D, B, A, F, C);
			ff("AECDBF", A, E, C, D, B, F);	ff("CEBDAF", C, E, B, D, A, F);	ff("EDBCAF", E, D, B, C, A, F);
			ff("AECDFB", A, E, C, D, F, B);	ff("CEBDFA", C, E, B, D, F, A);	ff("EDBCFA", E, D, B, C, F, A);
			ff("AECFBD", A, E, C, F, B, D);	ff("CEBFAD", C, E, B, F, A, D);	ff("EDBFAC", E, D, B, F, A, C);
			ff("AECFDB", A, E, C, F, D, B);	ff("CEBFDA", C, E, B, F, D, A);	ff("EDBFCA", E, D, B, F, C, A);
			ff("AEDBCF", A, E, D, B, C, F);	ff("CEDABF", C, E, D, A, B, F);	ff("EDCABF", E, D, C, A, B, F);
			ff("AEDBFC", A, E, D, B, F, C);	ff("CEDAFB", C, E, D, A, F, B);	ff("EDCAFB", E, D, C, A, F, B);
			ff("AEDCBF", A, E, D, C, B, F);	ff("CEDBAF", C, E, D, B, A, F);	ff("EDCBAF", E, D, C, B, A, F);
			ff("AEDCFB", A, E, D, C, F, B);	ff("CEDBFA", C, E, D, B, F, A);	ff("EDCBFA", E, D, C, B, F, A);
			ff("AEDFBC", A, E, D, F, B, C);	ff("CEDFAB", C, E, D, F, A, B);	ff("EDCFAB", E, D, C, F, A, B);
			ff("AEDFCB", A, E, D, F, C, B);	ff("CEDFBA", C, E, D, F, B, A);	ff("EDCFBA", E, D, C, F, B, A);
			ff("AEFBCD", A, E, F, B, C, D);	ff("CEFABD", C, E, F, A, B, D);	ff("EDFABC", E, D, F, A, B, C);
			ff("AEFBDC", A, E, F, B, D, C);	ff("CEFADB", C, E, F, A, D, B);	ff("EDFACB", E, D, F, A, C, B);
			ff("AEFCBD", A, E, F, C, B, D);	ff("CEFBAD", C, E, F, B, A, D);	ff("EDFBAC", E, D, F, B, A, C);
			ff("AEFCDB", A, E, F, C, D, B);	ff("CEFBDA", C, E, F, B, D, A);	ff("EDFBCA", E, D, F, B, C, A);
			ff("AEFDBC", A, E, F, D, B, C);	ff("CEFDAB", C, E, F, D, A, B);	ff("EDFCAB", E, D, F, C, A, B);
			ff("AEFDCB", A, E, F, D, C, B);	ff("CEFDBA", C, E, F, D, B, A);	ff("EDFCBA", E, D, F, C, B, A);
			ff("AFBCDE", A, F, B, C, D, E);	ff("CFABDE", C, F, A, B, D, E);	ff("EFABCD", E, F, A, B, C, D);
			ff("AFBCED", A, F, B, C, E, D);	ff("CFABED", C, F, A, B, E, D);	ff("EFABDC", E, F, A, B, D, C);
			ff("AFBDCE", A, F, B, D, C, E);	ff("CFADBE", C, F, A, D, B, E);	ff("EFACBD", E, F, A, C, B, D);
			ff("AFBDEC", A, F, B, D, E, C);	ff("CFADEB", C, F, A, D, E, B);	ff("EFACDB", E, F, A, C, D, B);
			ff("AFBECD", A, F, B, E, C, D);	ff("CFAEBD", C, F, A, E, B, D);	ff("EFADBC", E, F, A, D, B, C);
			ff("AFBEDC", A, F, B, E, D, C);	ff("CFAEDB", C, F, A, E, D, B);	ff("EFADCB", E, F, A, D, C, B);
			ff("AFCBDE", A, F, C, B, D, E);	ff("CFBADE", C, F, B, A, D, E);	ff("EFBACD", E, F, B, A, C, D);
			ff("AFCBED", A, F, C, B, E, D);	ff("CFBAED", C, F, B, A, E, D);	ff("EFBADC", E, F, B, A, D, C);
			ff("AFCDBE", A, F, C, D, B, E);	ff("CFBDAE", C, F, B, D, A, E);	ff("EFBCAD", E, F, B, C, A, D);
			ff("AFCDEB", A, F, C, D, E, B);	ff("CFBDEA", C, F, B, D, E, A);	ff("EFBCDA", E, F, B, C, D, A);
			ff("AFCEBD", A, F, C, E, B, D);	ff("CFBEAD", C, F, B, E, A, D);	ff("EFBDAC", E, F, B, D, A, C);
			ff("AFCEDB", A, F, C, E, D, B);	ff("CFBEDA", C, F, B, E, D, A);	ff("EFBDCA", E, F, B, D, C, A);
			ff("AFDBCE", A, F, D, B, C, E);	ff("CFDABE", C, F, D, A, B, E);	ff("EFCABD", E, F, C, A, B, D);
			ff("AFDBEC", A, F, D, B, E, C);	ff("CFDAEB", C, F, D, A, E, B);	ff("EFCADB", E, F, C, A, D, B);
			ff("AFDCBE", A, F, D, C, B, E);	ff("CFDBAE", C, F, D, B, A, E);	ff("EFCBAD", E, F, C, B, A, D);
			ff("AFDCEB", A, F, D, C, E, B);	ff("CFDBEA", C, F, D, B, E, A);	ff("EFCBDA", E, F, C, B, D, A);
			ff("AFDEBC", A, F, D, E, B, C);	ff("CFDEAB", C, F, D, E, A, B);	ff("EFCDAB", E, F, C, D, A, B);
			ff("AFDECB", A, F, D, E, C, B);	ff("CFDEBA", C, F, D, E, B, A);	ff("EFCDBA", E, F, C, D, B, A);
			ff("AFEBCD", A, F, E, B, C, D);	ff("CFEABD", C, F, E, A, B, D);	ff("EFDABC", E, F, D, A, B, C);
			ff("AFEBDC", A, F, E, B, D, C);	ff("CFEADB", C, F, E, A, D, B);	ff("EFDACB", E, F, D, A, C, B);
			ff("AFECBD", A, F, E, C, B, D);	ff("CFEBAD", C, F, E, B, A, D);	ff("EFDBAC", E, F, D, B, A, C);
			ff("AFECDB", A, F, E, C, D, B);	ff("CFEBDA", C, F, E, B, D, A);	ff("EFDBCA", E, F, D, B, C, A);
			ff("AFEDBC", A, F, E, D, B, C);	ff("CFEDAB", C, F, E, D, A, B);	ff("EFDCAB", E, F, D, C, A, B);
			ff("AFEDCB", A, F, E, D, C, B);	ff("CFEDBA", C, F, E, D, B, A);	ff("EFDCBA", E, F, D, C, B, A);

			ff("BACDEF", B, A, C, D, E, F);	ff("DABCEF", D, A, B, C, E, F);	ff("FABCDE", F, A, B, C, D, E);
			ff("BACDFE", B, A, C, D, F, E);	ff("DABCFE", D, A, B, C, F, E);	ff("FABCED", F, A, B, C, E, D);
			ff("BACEDF", B, A, C, E, D, F);	ff("DABECF", D, A, B, E, C, F);	ff("FABDCE", F, A, B, D, C, E);
			ff("BACEFD", B, A, C, E, F, D);	ff("DABEFC", D, A, B, E, F, C);	ff("FABDEC", F, A, B, D, E, C);
			ff("BACFDE", B, A, C, F, D, E);	ff("DABFCE", D, A, B, F, C, E);	ff("FABECD", F, A, B, E, C, D);
			ff("BACFED", B, A, C, F, E, D);	ff("DABFEC", D, A, B, F, E, C);	ff("FABEDC", F, A, B, E, D, C);
			ff("BADCEF", B, A, D, C, E, F);	ff("DACBEF", D, A, C, B, E, F);	ff("FACBDE", F, A, C, B, D, E);
			ff("BADCFE", B, A, D, C, F, E);	ff("DACBFE", D, A, C, B, F, E);	ff("FACBED", F, A, C, B, E, D);
			ff("BADECF", B, A, D, E, C, F);	ff("DACEBF", D, A, C, E, B, F);	ff("FACDBE", F, A, C, D, B, E);
			ff("BADEFC", B, A, D, E, F, C);	ff("DACEFB", D, A, C, E, F, B);	ff("FACDEB", F, A, C, D, E, B);
			ff("BADFCE", B, A, D, F, C, E);	ff("DACFBE", D, A, C, F, B, E);	ff("FACEBD", F, A, C, E, B, D);
			ff("BADFEC", B, A, D, F, E, C);	ff("DACFEB", D, A, C, F, E, B);	ff("FACEDB", F, A, C, E, D, B);
			ff("BAECDF", B, A, E, C, D, F);	ff("DAEBCF", D, A, E, B, C, F);	ff("FADBCE", F, A, D, B, C, E);
			ff("BAECFD", B, A, E, C, F, D);	ff("DAEBFC", D, A, E, B, F, C);	ff("FADBEC", F, A, D, B, E, C);
			ff("BAEDCF", B, A, E, D, C, F);	ff("DAECBF", D, A, E, C, B, F);	ff("FADCBE", F, A, D, C, B, E);
			ff("BAEDFC", B, A, E, D, F, C);	ff("DAECFB", D, A, E, C, F, B);	ff("FADCEB", F, A, D, C, E, B);
			ff("BAEFCD", B, A, E, F, C, D);	ff("DAEFBC", D, A, E, F, B, C);	ff("FADEBC", F, A, D, E, B, C);
			ff("BAEFDC", B, A, E, F, D, C);	ff("DAEFCB", D, A, E, F, C, B);	ff("FADECB", F, A, D, E, C, B);
			ff("BAFCDE", B, A, F, C, D, E);	ff("DAFBCE", D, A, F, B, C, E);	ff("FAEBCD", F, A, E, B, C, D);
			ff("BAFCED", B, A, F, C, E, D);	ff("DAFBEC", D, A, F, B, E, C);	ff("FAEBDC", F, A, E, B, D, C);
			ff("BAFDCE", B, A, F, D, C, E);	ff("DAFCBE", D, A, F, C, B, E);	ff("FAECBD", F, A, E, C, B, D);
			ff("BAFDEC", B, A, F, D, E, C);	ff("DAFCEB", D, A, F, C, E, B);	ff("FAECDB", F, A, E, C, D, B);
			ff("BAFECD", B, A, F, E, C, D);	ff("DAFEBC", D, A, F, E, B, C);	ff("FAEDBC", F, A, E, D, B, C);
			ff("BAFEDC", B, A, F, E, D, C);	ff("DAFECB", D, A, F, E, C, B);	ff("FAEDCB", F, A, E, D, C, B);
			ff("BCADEF", B, C, A, D, E, F);	ff("DBACEF", D, B, A, C, E, F);	ff("FBACDE", F, B, A, C, D, E);
			ff("BCADFE", B, C, A, D, F, E);	ff("DBACFE", D, B, A, C, F, E);	ff("FBACED", F, B, A, C, E, D);
			ff("BCAEDF", B, C, A, E, D, F);	ff("DBAECF", D, B, A, E, C, F);	ff("FBADCE", F, B, A, D, C, E);
			ff("BCAEFD", B, C, A, E, F, D);	ff("DBAEFC", D, B, A, E, F, C);	ff("FBADEC", F, B, A, D, E, C);
			ff("BCAFDE", B, C, A, F, D, E);	ff("DBAFCE", D, B, A, F, C, E);	ff("FBAECD", F, B, A, E, C, D);
			ff("BCAFED", B, C, A, F, E, D);	ff("DBAFEC", D, B, A, F, E, C);	ff("FBAEDC", F, B, A, E, D, C);
			ff("BCDAEF", B, C, D, A, E, F);	ff("DBCAEF", D, B, C, A, E, F);	ff("FBCADE", F, B, C, A, D, E);
			ff("BCDAFE", B, C, D, A, F, E);	ff("DBCAFE", D, B, C, A, F, E);	ff("FBCAED", F, B, C, A, E, D);
			ff("BCDEAF", B, C, D, E, A, F);	ff("DBCEAF", D, B, C, E, A, F);	ff("FBCDAE", F, B, C, D, A, E);
			ff("BCDEFA", B, C, D, E, F, A);	ff("DBCEFA", D, B, C, E, F, A);	ff("FBCDEA", F, B, C, D, E, A);
			ff("BCDFAE", B, C, D, F, A, E);	ff("DBCFAE", D, B, C, F, A, E);	ff("FBCEAD", F, B, C, E, A, D);
			ff("BCDFEA", B, C, D, F, E, A);	ff("DBCFEA", D, B, C, F, E, A);	ff("FBCEDA", F, B, C, E, D, A);
			ff("BCEADF", B, C, E, A, D, F);	ff("DBEACF", D, B, E, A, C, F);	ff("FBDACE", F, B, D, A, C, E);
			ff("BCEAFD", B, C, E, A, F, D);	ff("DBEAFC", D, B, E, A, F, C);	ff("FBDAEC", F, B, D, A, E, C);
			ff("BCEDAF", B, C, E, D, A, F);	ff("DBECAF", D, B, E, C, A, F);	ff("FBDCAE", F, B, D, C, A, E);
			ff("BCEDFA", B, C, E, D, F, A);	ff("DBECFA", D, B, E, C, F, A);	ff("FBDCEA", F, B, D, C, E, A);
			ff("BCEFAD", B, C, E, F, A, D);	ff("DBEFAC", D, B, E, F, A, C);	ff("FBDEAC", F, B, D, E, A, C);
			ff("BCEFDA", B, C, E, F, D, A);	ff("DBEFCA", D, B, E, F, C, A);	ff("FBDECA", F, B, D, E, C, A);
			ff("BCFADE", B, C, F, A, D, E);	ff("DBFACE", D, B, F, A, C, E);	ff("FBEACD", F, B, E, A, C, D);
			ff("BCFAED", B, C, F, A, E, D);	ff("DBFAEC", D, B, F, A, E, C);	ff("FBEADC", F, B, E, A, D, C);
			ff("BCFDAE", B, C, F, D, A, E);	ff("DBFCAE", D, B, F, C, A, E);	ff("FBECAD", F, B, E, C, A, D);
			ff("BCFDEA", B, C, F, D, E, A);	ff("DBFCEA", D, B, F, C, E, A);	ff("FBECDA", F, B, E, C, D, A);
			ff("BCFEAD", B, C, F, E, A, D);	ff("DBFEAC", D, B, F, E, A, C);	ff("FBEDAC", F, B, E, D, A, C);
			ff("BCFEDA", B, C, F, E, D, A);	ff("DBFECA", D, B, F, E, C, A);	ff("FBEDCA", F, B, E, D, C, A);
			ff("BDACEF", B, D, A, C, E, F);	ff("DCABEF", D, C, A, B, E, F);	ff("FCABDE", F, C, A, B, D, E);
			ff("BDACFE", B, D, A, C, F, E);	ff("DCABFE", D, C, A, B, F, E);	ff("FCABED", F, C, A, B, E, D);
			ff("BDAECF", B, D, A, E, C, F);	ff("DCAEBF", D, C, A, E, B, F);	ff("FCADBE", F, C, A, D, B, E);
			ff("BDAEFC", B, D, A, E, F, C);	ff("DCAEFB", D, C, A, E, F, B);	ff("FCADEB", F, C, A, D, E, B);
			ff("BDAFCE", B, D, A, F, C, E);	ff("DCAFBE", D, C, A, F, B, E);	ff("FCAEBD", F, C, A, E, B, D);
			ff("BDAFEC", B, D, A, F, E, C);	ff("DCAFEB", D, C, A, F, E, B);	ff("FCAEDB", F, C, A, E, D, B);
			ff("BDCAEF", B, D, C, A, E, F);	ff("DCBAEF", D, C, B, A, E, F);	ff("FCBADE", F, C, B, A, D, E);
			ff("BDCAFE", B, D, C, A, F, E);	ff("DCBAFE", D, C, B, A, F, E);	ff("FCBAED", F, C, B, A, E, D);
			ff("BDCEAF", B, D, C, E, A, F);	ff("DCBEAF", D, C, B, E, A, F);	ff("FCBDAE", F, C, B, D, A, E);
			ff("BDCEFA", B, D, C, E, F, A);	ff("DCBEFA", D, C, B, E, F, A);	ff("FCBDEA", F, C, B, D, E, A);
			ff("BDCFAE", B, D, C, F, A, E);	ff("DCBFAE", D, C, B, F, A, E);	ff("FCBEAD", F, C, B, E, A, D);
			ff("BDCFEA", B, D, C, F, E, A);	ff("DCBFEA", D, C, B, F, E, A);	ff("FCBEDA", F, C, B, E, D, A);
			ff("BDEACF", B, D, E, A, C, F);	ff("DCEABF", D, C, E, A, B, F);	ff("FCDABE", F, C, D, A, B, E);
			ff("BDEAFC", B, D, E, A, F, C);	ff("DCEAFB", D, C, E, A, F, B);	ff("FCDAEB", F, C, D, A, E, B);
			ff("BDECAF", B, D, E, C, A, F);	ff("DCEBAF", D, C, E, B, A, F);	ff("FCDBAE", F, C, D, B, A, E);
			ff("BDECFA", B, D, E, C, F, A);	ff("DCEBFA", D, C, E, B, F, A);	ff("FCDBEA", F, C, D, B, E, A);
			ff("BDEFAC", B, D, E, F, A, C);	ff("DCEFAB", D, C, E, F, A, B);	ff("FCDEAB", F, C, D, E, A, B);
			ff("BDEFCA", B, D, E, F, C, A);	ff("DCEFBA", D, C, E, F, B, A);	ff("FCDEBA", F, C, D, E, B, A);
			ff("BDFACE", B, D, F, A, C, E);	ff("DCFABE", D, C, F, A, B, E);	ff("FCEABD", F, C, E, A, B, D);
			ff("BDFAEC", B, D, F, A, E, C);	ff("DCFAEB", D, C, F, A, E, B);	ff("FCEADB", F, C, E, A, D, B);
			ff("BDFCAE", B, D, F, C, A, E);	ff("DCFBAE", D, C, F, B, A, E);	ff("FCEBAD", F, C, E, B, A, D);
			ff("BDFCEA", B, D, F, C, E, A);	ff("DCFBEA", D, C, F, B, E, A);	ff("FCEBDA", F, C, E, B, D, A);
			ff("BDFEAC", B, D, F, E, A, C);	ff("DCFEAB", D, C, F, E, A, B);	ff("FCEDAB", F, C, E, D, A, B);
			ff("BDFECA", B, D, F, E, C, A);	ff("DCFEBA", D, C, F, E, B, A);	ff("FCEDBA", F, C, E, D, B, A);
			ff("BEACDF", B, E, A, C, D, F);	ff("DEABCF", D, E, A, B, C, F);	ff("FDABCE", F, D, A, B, C, E);
			ff("BEACFD", B, E, A, C, F, D);	ff("DEABFC", D, E, A, B, F, C);	ff("FDABEC", F, D, A, B, E, C);
			ff("BEADCF", B, E, A, D, C, F);	ff("DEACBF", D, E, A, C, B, F);	ff("FDACBE", F, D, A, C, B, E);
			ff("BEADFC", B, E, A, D, F, C);	ff("DEACFB", D, E, A, C, F, B);	ff("FDACEB", F, D, A, C, E, B);
			ff("BEAFCD", B, E, A, F, C, D);	ff("DEAFBC", D, E, A, F, B, C);	ff("FDAEBC", F, D, A, E, B, C);
			ff("BEAFDC", B, E, A, F, D, C);	ff("DEAFCB", D, E, A, F, C, B);	ff("FDAECB", F, D, A, E, C, B);
			ff("BECADF", B, E, C, A, D, F);	ff("DEBACF", D, E, B, A, C, F);	ff("FDBACE", F, D, B, A, C, E);
			ff("BECAFD", B, E, C, A, F, D);	ff("DEBAFC", D, E, B, A, F, C);	ff("FDBAEC", F, D, B, A, E, C);
			ff("BECDAF", B, E, C, D, A, F);	ff("DEBCAF", D, E, B, C, A, F);	ff("FDBCAE", F, D, B, C, A, E);
			ff("BECDFA", B, E, C, D, F, A);	ff("DEBCFA", D, E, B, C, F, A);	ff("FDBCEA", F, D, B, C, E, A);
			ff("BECFAD", B, E, C, F, A, D);	ff("DEBFAC", D, E, B, F, A, C);	ff("FDBEAC", F, D, B, E, A, C);
			ff("BECFDA", B, E, C, F, D, A);	ff("DEBFCA", D, E, B, F, C, A);	ff("FDBECA", F, D, B, E, C, A);
			ff("BEDACF", B, E, D, A, C, F);	ff("DECABF", D, E, C, A, B, F);	ff("FDCABE", F, D, C, A, B, E);
			ff("BEDAFC", B, E, D, A, F, C);	ff("DECAFB", D, E, C, A, F, B);	ff("FDCAEB", F, D, C, A, E, B);
			ff("BEDCAF", B, E, D, C, A, F);	ff("DECBAF", D, E, C, B, A, F);	ff("FDCBAE", F, D, C, B, A, E);
			ff("BEDCFA", B, E, D, C, F, A);	ff("DECBFA", D, E, C, B, F, A);	ff("FDCBEA", F, D, C, B, E, A);
			ff("BEDFAC", B, E, D, F, A, C);	ff("DECFAB", D, E, C, F, A, B);	ff("FDCEAB", F, D, C, E, A, B);
			ff("BEDFCA", B, E, D, F, C, A);	ff("DECFBA", D, E, C, F, B, A);	ff("FDCEBA", F, D, C, E, B, A);
			ff("BEFACD", B, E, F, A, C, D);	ff("DEFABC", D, E, F, A, B, C);	ff("FDEABC", F, D, E, A, B, C);
			ff("BEFADC", B, E, F, A, D, C);	ff("DEFACB", D, E, F, A, C, B);	ff("FDEACB", F, D, E, A, C, B);
			ff("BEFCAD", B, E, F, C, A, D);	ff("DEFBAC", D, E, F, B, A, C);	ff("FDEBAC", F, D, E, B, A, C);
			ff("BEFCDA", B, E, F, C, D, A);	ff("DEFBCA", D, E, F, B, C, A);	ff("FDEBCA", F, D, E, B, C, A);
			ff("BEFDAC", B, E, F, D, A, C);	ff("DEFCAB", D, E, F, C, A, B);	ff("FDECAB", F, D, E, C, A, B);
			ff("BEFDCA", B, E, F, D, C, A);	ff("DEFCBA", D, E, F, C, B, A);	ff("FDECBA", F, D, E, C, B, A);
			ff("BFACDE", B, F, A, C, D, E);	ff("DFABCE", D, F, A, B, C, E);	ff("FEABCD", F, E, A, B, C, D);
			ff("BFACED", B, F, A, C, E, D);	ff("DFABEC", D, F, A, B, E, C);	ff("FEABDC", F, E, A, B, D, C);
			ff("BFADCE", B, F, A, D, C, E);	ff("DFACBE", D, F, A, C, B, E);	ff("FEACBD", F, E, A, C, B, D);
			ff("BFADEC", B, F, A, D, E, C);	ff("DFACEB", D, F, A, C, E, B);	ff("FEACDB", F, E, A, C, D, B);
			ff("BFAECD", B, F, A, E, C, D);	ff("DFAEBC", D, F, A, E, B, C);	ff("FEADBC", F, E, A, D, B, C);
			ff("BFAEDC", B, F, A, E, D, C);	ff("DFAECB", D, F, A, E, C, B);	ff("FEADCB", F, E, A, D, C, B);
			ff("BFCADE", B, F, C, A, D, E);	ff("DFBACE", D, F, B, A, C, E);	ff("FEBACD", F, E, B, A, C, D);
			ff("BFCAED", B, F, C, A, E, D);	ff("DFBAEC", D, F, B, A, E, C);	ff("FEBADC", F, E, B, A, D, C);
			ff("BFCDAE", B, F, C, D, A, E);	ff("DFBCAE", D, F, B, C, A, E);	ff("FEBCAD", F, E, B, C, A, D);
			ff("BFCDEA", B, F, C, D, E, A);	ff("DFBCEA", D, F, B, C, E, A);	ff("FEBCDA", F, E, B, C, D, A);
			ff("BFCEAD", B, F, C, E, A, D);	ff("DFBEAC", D, F, B, E, A, C);	ff("FEBDAC", F, E, B, D, A, C);
			ff("BFCEDA", B, F, C, E, D, A);	ff("DFBECA", D, F, B, E, C, A);	ff("FEBDCA", F, E, B, D, C, A);
			ff("BFDACE", B, F, D, A, C, E);	ff("DFCABE", D, F, C, A, B, E);	ff("FECABD", F, E, C, A, B, D);
			ff("BFDAEC", B, F, D, A, E, C);	ff("DFCAEB", D, F, C, A, E, B);	ff("FECADB", F, E, C, A, D, B);
			ff("BFDCAE", B, F, D, C, A, E);	ff("DFCBAE", D, F, C, B, A, E);	ff("FECBAD", F, E, C, B, A, D);
			ff("BFDCEA", B, F, D, C, E, A);	ff("DFCBEA", D, F, C, B, E, A);	ff("FECBDA", F, E, C, B, D, A);
			ff("BFDEAC", B, F, D, E, A, C);	ff("DFCEAB", D, F, C, E, A, B);	ff("FECDAB", F, E, C, D, A, B);
			ff("BFDECA", B, F, D, E, C, A);	ff("DFCEBA", D, F, C, E, B, A);	ff("FECDBA", F, E, C, D, B, A);
			ff("BFEACD", B, F, E, A, C, D);	ff("DFEABC", D, F, E, A, B, C);	ff("FEDABC", F, E, D, A, B, C);
			ff("BFEADC", B, F, E, A, D, C);	ff("DFEACB", D, F, E, A, C, B);	ff("FEDACB", F, E, D, A, C, B);
			ff("BFECAD", B, F, E, C, A, D);	ff("DFEBAC", D, F, E, B, A, C);	ff("FEDBAC", F, E, D, B, A, C);
			ff("BFECDA", B, F, E, C, D, A);	ff("DFEBCA", D, F, E, B, C, A);	ff("FEDBCA", F, E, D, B, C, A);
			ff("BFEDAC", B, F, E, D, A, C);	ff("DFECAB", D, F, E, C, A, B);	ff("FEDCAB", F, E, D, C, A, B);
			ff("BFEDCA", B, F, E, D, C, A);	ff("DFECBA", D, F, E, C, B, A);	ff("FEDCBA", F, E, D, C, B, A);
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

