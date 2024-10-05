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
				(void) separator;

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

			auto _ = [&](std::string_view txt, std::string_view a){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, keySub);

				func(key);
			};

			// old style not supports txt
			_("X", A);
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

			auto _ = [&](std::string_view txt, std::string_view a){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, keySub);

				func(key);
			};

			_("A", A);
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

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, keySub);

				func(key);
			};

			_("AB", A, B);
			_("BA", B, A);
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

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, keySub);

				func(key);
			};

			_("ABC", A, B, C);
			_("ACB", A, C, B);
			_("BAC", B, A, C);
			_("BCA", B, C, A);
			_("CAB", C, A, B);
			_("CBA", C, B, A);

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

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, keySub);

				func(key);
			};

			_("ABCD", A, B, C, D);
			_("ABDC", A, B, D, C);
			_("ACBD", A, C, B, D);
			_("ACDB", A, C, D, B);
			_("ADBC", A, D, B, C);
			_("ADCB", A, D, C, B);
			_("BACD", B, A, C, D);
			_("BADC", B, A, D, C);
			_("BCAD", B, C, A, D);
			_("BCDA", B, C, D, A);
			_("BDAC", B, D, A, C);
			_("BDCA", B, D, C, A);
			_("CABD", C, A, B, D);
			_("CADB", C, A, D, B);
			_("CBAD", C, B, A, D);
			_("CBDA", C, B, D, A);
			_("CDAB", C, D, A, B);
			_("CDBA", C, D, B, A);
			_("DABC", D, A, B, C);
			_("DACB", D, A, C, B);
			_("DBAC", D, B, A, C);
			_("DBCA", D, B, C, A);
			_("DCAB", D, C, A, B);
			_("DCBA", D, C, B, A);
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

			auto _ = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, keySub);

				func(key);
			};

			// All 120 permutations here:
			// thanks chatgpt

			_("ABCDE", A, B, C, D, E);
			_("ABCED", A, B, C, E, D);
			_("ABDCE", A, B, D, C, E);
			_("ABDEC", A, B, D, E, C);
			_("ABECD", A, B, E, C, D);
			_("ABEDC", A, B, E, D, C);
			_("ACBDE", A, C, B, D, E);
			_("ACBED", A, C, B, E, D);
			_("ACDBE", A, C, D, B, E);
			_("ACDEB", A, C, D, E, B);
			_("ACEBD", A, C, E, B, D);
			_("ACEDC", A, C, E, D, C);
			_("ADBCE", A, D, B, C, E);
			_("ADBEC", A, D, B, E, C);
			_("ADCBE", A, D, C, B, E);
			_("ADCEB", A, D, C, E, B);
			_("ADEBC", A, D, E, B, C);
			_("ADECB", A, D, E, C, B);
			_("AEBCD", A, E, B, C, D);
			_("AEBDC", A, E, B, D, C);
			_("AECBD", A, E, C, B, D);
			_("AECDB", A, E, C, D, B);
			_("AEDBC", A, E, D, B, C);
			_("AEDCB", A, E, D, C, B);
			_("BACDE", B, A, C, D, E);
			_("BACED", B, A, C, E, D);
			_("BADCE", B, A, D, C, E);
			_("BADEC", B, A, D, E, C);
			_("BAECD", B, A, E, C, D);
			_("BAEDC", B, A, E, D, C);
			_("BCADE", B, C, A, D, E);
			_("BCAED", B, C, A, E, D);
			_("BCDAE", B, C, D, A, E);
			_("BCDEA", B, C, D, E, A);
			_("BCEAD", B, C, E, A, D);
			_("BCEDA", B, C, E, D, A);
			_("BDACE", B, D, A, C, E);
			_("BDAEC", B, D, A, E, C);
			_("BDCAE", B, D, C, A, E);
			_("BDCEA", B, D, C, E, A);
			_("BDEAC", B, D, E, A, C);
			_("BDECA", B, D, E, C, A);
			_("BEACD", B, E, A, C, D);
			_("BEADC", B, E, A, D, C);
			_("BECAD", B, E, C, A, D);
			_("BECDA", B, E, C, D, A);
			_("BEDAC", B, E, D, A, C);
			_("BEDCA", B, E, D, C, A);
			_("CABDE", C, A, B, D, E);
			_("CABED", C, A, B, E, D);
			_("CADBE", C, A, D, B, E);
			_("CADEB", C, A, D, E, B);
			_("CAEBD", C, A, E, B, D);
			_("CAEDB", C, A, E, D, B);
			_("CBADE", C, B, A, D, E);
			_("CBAED", C, B, A, E, D);
			_("CBDAE", C, B, D, A, E);
			_("CBDEA", C, B, D, E, A);
			_("CBEAD", C, B, E, A, D);
			_("CBEDA", C, B, E, D, A);
			_("CDABE", C, D, A, B, E);
			_("CDAEB", C, D, A, E, B);
			_("CDBAE", C, D, B, A, E);
			_("CDBEA", C, D, B, E, A);
			_("CDEAB", C, D, E, A, B);
			_("CDEBA", C, D, E, B, A);
			_("CEABD", C, E, A, B, D);
			_("CEADB", C, E, A, D, B);
			_("CEBAD", C, E, B, A, D);
			_("CEBDA", C, E, B, D, A);
			_("CEDAB", C, E, D, A, B);
			_("CEDBA", C, E, D, B, A);
			_("DABCE", D, A, B, C, E);
			_("DABEC", D, A, B, E, C);
			_("DACBE", D, A, C, B, E);
			_("DACEB", D, A, C, E, B);
			_("DAEBC", D, A, E, B, C);
			_("DAECB", D, A, E, C, B);
			_("DBACE", D, B, A, C, E);
			_("DBAEC", D, B, A, E, C);
			_("DBCAE", D, B, C, A, E);
			_("DBCEA", D, B, C, E, A);
			_("DBEAC", D, B, E, A, C);
			_("DBECA", D, B, E, C, A);
			_("DCABE", D, C, A, B, E);
			_("DCAEB", D, C, A, E, B);
			_("DCBAE", D, C, B, A, E);
			_("DCBEA", D, C, B, E, A);
			_("DCEAB", D, C, E, A, B);
			_("DCEBA", D, C, E, B, A);
			_("DEABC", D, E, A, B, C);
			_("DEACB", D, E, A, C, B);
			_("DEBAC", D, E, B, A, C);
			_("DEBCA", D, E, B, C, A);
			_("DECAB", D, E, C, A, B);
			_("DECBA", D, E, C, B, A);
			_("EABCD", E, A, B, C, D);
			_("EABDC", E, A, B, D, C);
			_("EACBD", E, A, C, B, D);
			_("EACDB", E, A, C, D, B);
			_("EADBC", E, A, D, B, C);
			_("EADCB", E, A, D, C, B);
			_("EBACD", E, B, A, C, D);
			_("EBADC", E, B, A, D, C);
			_("EBCAD", E, B, C, A, D);
			_("EBCDA", E, B, C, D, A);
			_("EBDAC", E, B, D, A, C);
			_("EBDCA", E, B, D, C, A);
			_("ECABD", E, C, A, B, D);
			_("ECADB", E, C, A, D, B);
			_("ECBAD", E, C, B, A, D);
			_("ECBDA", E, C, B, D, A);
			_("ECDAB", E, C, D, A, B);
			_("ECDBA", E, C, D, B, A);
			_("EDABC", E, D, A, B, C);
			_("EDACB", E, D, A, C, B);
			_("EDBAC", E, D, B, A, C);
			_("EDBCA", E, D, B, C, A);
			_("EDCAB", E, D, C, A, B);
			_("EDCBA", E, D, C, B, A);
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

			auto _ = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e, std::string_view f){

				hm4::PairBufferKey bufferKey;

				auto const key = makeKey(bufferKey, separator, keyN, txt, a, b, c, d, e, f, keySub);

				func(key);
			};

			// All 720 permutations here:
			// thanks wolfram alpha
			// this is still acceptable, sinse XNDEL can do 32K inserts.

			_("ABCDEF", A, B, C, D, E, F);	_("CABDEF", C, A, B, D, E, F);	_("EABCDF", E, A, B, C, D, F);
			_("ABCDFE", A, B, C, D, F, E);	_("CABDFE", C, A, B, D, F, E);	_("EABCFD", E, A, B, C, F, D);
			_("ABCEDF", A, B, C, E, D, F);	_("CABEDF", C, A, B, E, D, F);	_("EABDCF", E, A, B, D, C, F);
			_("ABCEFD", A, B, C, E, F, D);	_("CABEFD", C, A, B, E, F, D);	_("EABDFC", E, A, B, D, F, C);
			_("ABCFDE", A, B, C, F, D, E);	_("CABFDE", C, A, B, F, D, E);	_("EABFCD", E, A, B, F, C, D);
			_("ABCFED", A, B, C, F, E, D);	_("CABFED", C, A, B, F, E, D);	_("EABFDC", E, A, B, F, D, C);
			_("ABDCEF", A, B, D, C, E, F);	_("CADBEF", C, A, D, B, E, F);	_("EACBDF", E, A, C, B, D, F);
			_("ABDCFE", A, B, D, C, F, E);	_("CADBFE", C, A, D, B, F, E);	_("EACBFD", E, A, C, B, F, D);
			_("ABDECF", A, B, D, E, C, F);	_("CADEBF", C, A, D, E, B, F);	_("EACDBF", E, A, C, D, B, F);
			_("ABDEFC", A, B, D, E, F, C);	_("CADEFB", C, A, D, E, F, B);	_("EACDFB", E, A, C, D, F, B);
			_("ABDFCE", A, B, D, F, C, E);	_("CADFBE", C, A, D, F, B, E);	_("EACFBD", E, A, C, F, B, D);
			_("ABDFEC", A, B, D, F, E, C);	_("CADFEB", C, A, D, F, E, B);	_("EACFDB", E, A, C, F, D, B);
			_("ABECDF", A, B, E, C, D, F);	_("CAEBDF", C, A, E, B, D, F);	_("EADBCF", E, A, D, B, C, F);
			_("ABECFD", A, B, E, C, F, D);	_("CAEBFD", C, A, E, B, F, D);	_("EADBFC", E, A, D, B, F, C);
			_("ABEDCF", A, B, E, D, C, F);	_("CAEDBF", C, A, E, D, B, F);	_("EADCBF", E, A, D, C, B, F);
			_("ABEDFC", A, B, E, D, F, C);	_("CAEDFB", C, A, E, D, F, B);	_("EADCFB", E, A, D, C, F, B);
			_("ABEFCD", A, B, E, F, C, D);	_("CAEFBD", C, A, E, F, B, D);	_("EADFBC", E, A, D, F, B, C);
			_("ABEFDC", A, B, E, F, D, C);	_("CAEFDB", C, A, E, F, D, B);	_("EADFCB", E, A, D, F, C, B);
			_("ABFCDE", A, B, F, C, D, E);	_("CAFBDE", C, A, F, B, D, E);	_("EAFBCD", E, A, F, B, C, D);
			_("ABFCED", A, B, F, C, E, D);	_("CAFBED", C, A, F, B, E, D);	_("EAFBDC", E, A, F, B, D, C);
			_("ABFDCE", A, B, F, D, C, E);	_("CAFDBE", C, A, F, D, B, E);	_("EAFCBD", E, A, F, C, B, D);
			_("ABFDEC", A, B, F, D, E, C);	_("CAFDEB", C, A, F, D, E, B);	_("EAFCDB", E, A, F, C, D, B);
			_("ABFECD", A, B, F, E, C, D);	_("CAFEBD", C, A, F, E, B, D);	_("EAFDBC", E, A, F, D, B, C);
			_("ABFEDC", A, B, F, E, D, C);	_("CAFEDB", C, A, F, E, D, B);	_("EAFDCB", E, A, F, D, C, B);
			_("ACBDEF", A, C, B, D, E, F);	_("CBADEF", C, B, A, D, E, F);	_("EBACDF", E, B, A, C, D, F);
			_("ACBDFE", A, C, B, D, F, E);	_("CBADFE", C, B, A, D, F, E);	_("EBACFD", E, B, A, C, F, D);
			_("ACBEDF", A, C, B, E, D, F);	_("CBAEDF", C, B, A, E, D, F);	_("EBADCF", E, B, A, D, C, F);
			_("ACBEFD", A, C, B, E, F, D);	_("CBAEFD", C, B, A, E, F, D);	_("EBADFC", E, B, A, D, F, C);
			_("ACBFDE", A, C, B, F, D, E);	_("CBAFDE", C, B, A, F, D, E);	_("EBAFCD", E, B, A, F, C, D);
			_("ACBFED", A, C, B, F, E, D);	_("CBAFED", C, B, A, F, E, D);	_("EBAFDC", E, B, A, F, D, C);
			_("ACDBEF", A, C, D, B, E, F);	_("CBDAEF", C, B, D, A, E, F);	_("EBCADF", E, B, C, A, D, F);
			_("ACDBFE", A, C, D, B, F, E);	_("CBDAFE", C, B, D, A, F, E);	_("EBCAFD", E, B, C, A, F, D);
			_("ACDEBF", A, C, D, E, B, F);	_("CBDEAF", C, B, D, E, A, F);	_("EBCDAF", E, B, C, D, A, F);
			_("ACDEFB", A, C, D, E, F, B);	_("CBDEFA", C, B, D, E, F, A);	_("EBCDFA", E, B, C, D, F, A);
			_("ACDFBE", A, C, D, F, B, E);	_("CBDFAE", C, B, D, F, A, E);	_("EBCFAD", E, B, C, F, A, D);
			_("ACDFEB", A, C, D, F, E, B);	_("CBDFEA", C, B, D, F, E, A);	_("EBCFDA", E, B, C, F, D, A);
			_("ACEBDF", A, C, E, B, D, F);	_("CBEADF", C, B, E, A, D, F);	_("EBDACF", E, B, D, A, C, F);
			_("ACEBFD", A, C, E, B, F, D);	_("CBEAFD", C, B, E, A, F, D);	_("EBDAFC", E, B, D, A, F, C);
			_("ACEDBF", A, C, E, D, B, F);	_("CBEDAF", C, B, E, D, A, F);	_("EBDCAF", E, B, D, C, A, F);
			_("ACEDFB", A, C, E, D, F, B);	_("CBEDFA", C, B, E, D, F, A);	_("EBDCFA", E, B, D, C, F, A);
			_("ACEFBD", A, C, E, F, B, D);	_("CBEFAD", C, B, E, F, A, D);	_("EBDFAC", E, B, D, F, A, C);
			_("ACEFDB", A, C, E, F, D, B);	_("CBEFDA", C, B, E, F, D, A);	_("EBDFCA", E, B, D, F, C, A);
			_("ACFBDE", A, C, F, B, D, E);	_("CBFADE", C, B, F, A, D, E);	_("EBFACD", E, B, F, A, C, D);
			_("ACFBED", A, C, F, B, E, D);	_("CBFAED", C, B, F, A, E, D);	_("EBFADC", E, B, F, A, D, C);
			_("ACFDBE", A, C, F, D, B, E);	_("CBFDAE", C, B, F, D, A, E);	_("EBFCAD", E, B, F, C, A, D);
			_("ACFDEB", A, C, F, D, E, B);	_("CBFDEA", C, B, F, D, E, A);	_("EBFCDA", E, B, F, C, D, A);
			_("ACFEBD", A, C, F, E, B, D);	_("CBFEAD", C, B, F, E, A, D);	_("EBFDAC", E, B, F, D, A, C);
			_("ACFEDB", A, C, F, E, D, B);	_("CBFEDA", C, B, F, E, D, A);	_("EBFDCA", E, B, F, D, C, A);
			_("ADBCEF", A, D, B, C, E, F);	_("CDABEF", C, D, A, B, E, F);	_("ECABDF", E, C, A, B, D, F);
			_("ADBCFE", A, D, B, C, F, E);	_("CDABFE", C, D, A, B, F, E);	_("ECABFD", E, C, A, B, F, D);
			_("ADBECF", A, D, B, E, C, F);	_("CDAEBF", C, D, A, E, B, F);	_("ECADBF", E, C, A, D, B, F);
			_("ADBEFC", A, D, B, E, F, C);	_("CDAEFB", C, D, A, E, F, B);	_("ECADFB", E, C, A, D, F, B);
			_("ADBFCE", A, D, B, F, C, E);	_("CDAFBE", C, D, A, F, B, E);	_("ECAFBD", E, C, A, F, B, D);
			_("ADBFEC", A, D, B, F, E, C);	_("CDAFEB", C, D, A, F, E, B);	_("ECAFDB", E, C, A, F, D, B);
			_("ADCBEF", A, D, C, B, E, F);	_("CDBAEF", C, D, B, A, E, F);	_("ECBADF", E, C, B, A, D, F);
			_("ADCBFE", A, D, C, B, F, E);	_("CDBAFE", C, D, B, A, F, E);	_("ECBAFD", E, C, B, A, F, D);
			_("ADCEBF", A, D, C, E, B, F);	_("CDBEAF", C, D, B, E, A, F);	_("ECBDAF", E, C, B, D, A, F);
			_("ADCEFB", A, D, C, E, F, B);	_("CDBEFA", C, D, B, E, F, A);	_("ECBDFA", E, C, B, D, F, A);
			_("ADCFBE", A, D, C, F, B, E);	_("CDBFAE", C, D, B, F, A, E);	_("ECBFAD", E, C, B, F, A, D);
			_("ADCFEB", A, D, C, F, E, B);	_("CDBFEA", C, D, B, F, E, A);	_("ECBFDA", E, C, B, F, D, A);
			_("ADEBCF", A, D, E, B, C, F);	_("CDEABF", C, D, E, A, B, F);	_("ECDABF", E, C, D, A, B, F);
			_("ADEBFC", A, D, E, B, F, C);	_("CDEAFB", C, D, E, A, F, B);	_("ECDAFB", E, C, D, A, F, B);
			_("ADECBF", A, D, E, C, B, F);	_("CDEBAF", C, D, E, B, A, F);	_("ECDBAF", E, C, D, B, A, F);
			_("ADECFB", A, D, E, C, F, B);	_("CDEBFA", C, D, E, B, F, A);	_("ECDBFA", E, C, D, B, F, A);
			_("ADEFBC", A, D, E, F, B, C);	_("CDEFAB", C, D, E, F, A, B);	_("ECDFAB", E, C, D, F, A, B);
			_("ADEFCB", A, D, E, F, C, B);	_("CDEFBA", C, D, E, F, B, A);	_("ECDFBA", E, C, D, F, B, A);
			_("ADFBCE", A, D, F, B, C, E);	_("CDFABE", C, D, F, A, B, E);	_("ECFABD", E, C, F, A, B, D);
			_("ADFBEC", A, D, F, B, E, C);	_("CDFAEB", C, D, F, A, E, B);	_("ECFADB", E, C, F, A, D, B);
			_("ADFCBE", A, D, F, C, B, E);	_("CDFBAE", C, D, F, B, A, E);	_("ECFBAD", E, C, F, B, A, D);
			_("ADFCEB", A, D, F, C, E, B);	_("CDFBEA", C, D, F, B, E, A);	_("ECFBDA", E, C, F, B, D, A);
			_("ADFEBC", A, D, F, E, B, C);	_("CDFEAB", C, D, F, E, A, B);	_("ECFDAB", E, C, F, D, A, B);
			_("ADFECB", A, D, F, E, C, B);	_("CDFEBA", C, D, F, E, B, A);	_("ECFDBA", E, C, F, D, B, A);
			_("AEBCDF", A, E, B, C, D, F);	_("CEABDF", C, E, A, B, D, F);	_("EDABCF", E, D, A, B, C, F);
			_("AEBCFD", A, E, B, C, F, D);	_("CEABFD", C, E, A, B, F, D);	_("EDABFC", E, D, A, B, F, C);
			_("AEBDCF", A, E, B, D, C, F);	_("CEADBF", C, E, A, D, B, F);	_("EDACBF", E, D, A, C, B, F);
			_("AEBDFC", A, E, B, D, F, C);	_("CEADFB", C, E, A, D, F, B);	_("EDACFB", E, D, A, C, F, B);
			_("AEBFCD", A, E, B, F, C, D);	_("CEAFBD", C, E, A, F, B, D);	_("EDAFBC", E, D, A, F, B, C);
			_("AEBFDC", A, E, B, F, D, C);	_("CEAFDB", C, E, A, F, D, B);	_("EDAFCB", E, D, A, F, C, B);
			_("AECBDF", A, E, C, B, D, F);	_("CEBADF", C, E, B, A, D, F);	_("EDBACF", E, D, B, A, C, F);
			_("AECBFD", A, E, C, B, F, D);	_("CEBAFD", C, E, B, A, F, D);	_("EDBAFC", E, D, B, A, F, C);
			_("AECDBF", A, E, C, D, B, F);	_("CEBDAF", C, E, B, D, A, F);	_("EDBCAF", E, D, B, C, A, F);
			_("AECDFB", A, E, C, D, F, B);	_("CEBDFA", C, E, B, D, F, A);	_("EDBCFA", E, D, B, C, F, A);
			_("AECFBD", A, E, C, F, B, D);	_("CEBFAD", C, E, B, F, A, D);	_("EDBFAC", E, D, B, F, A, C);
			_("AECFDB", A, E, C, F, D, B);	_("CEBFDA", C, E, B, F, D, A);	_("EDBFCA", E, D, B, F, C, A);
			_("AEDBCF", A, E, D, B, C, F);	_("CEDABF", C, E, D, A, B, F);	_("EDCABF", E, D, C, A, B, F);
			_("AEDBFC", A, E, D, B, F, C);	_("CEDAFB", C, E, D, A, F, B);	_("EDCAFB", E, D, C, A, F, B);
			_("AEDCBF", A, E, D, C, B, F);	_("CEDBAF", C, E, D, B, A, F);	_("EDCBAF", E, D, C, B, A, F);
			_("AEDCFB", A, E, D, C, F, B);	_("CEDBFA", C, E, D, B, F, A);	_("EDCBFA", E, D, C, B, F, A);
			_("AEDFBC", A, E, D, F, B, C);	_("CEDFAB", C, E, D, F, A, B);	_("EDCFAB", E, D, C, F, A, B);
			_("AEDFCB", A, E, D, F, C, B);	_("CEDFBA", C, E, D, F, B, A);	_("EDCFBA", E, D, C, F, B, A);
			_("AEFBCD", A, E, F, B, C, D);	_("CEFABD", C, E, F, A, B, D);	_("EDFABC", E, D, F, A, B, C);
			_("AEFBDC", A, E, F, B, D, C);	_("CEFADB", C, E, F, A, D, B);	_("EDFACB", E, D, F, A, C, B);
			_("AEFCBD", A, E, F, C, B, D);	_("CEFBAD", C, E, F, B, A, D);	_("EDFBAC", E, D, F, B, A, C);
			_("AEFCDB", A, E, F, C, D, B);	_("CEFBDA", C, E, F, B, D, A);	_("EDFBCA", E, D, F, B, C, A);
			_("AEFDBC", A, E, F, D, B, C);	_("CEFDAB", C, E, F, D, A, B);	_("EDFCAB", E, D, F, C, A, B);
			_("AEFDCB", A, E, F, D, C, B);	_("CEFDBA", C, E, F, D, B, A);	_("EDFCBA", E, D, F, C, B, A);
			_("AFBCDE", A, F, B, C, D, E);	_("CFABDE", C, F, A, B, D, E);	_("EFABCD", E, F, A, B, C, D);
			_("AFBCED", A, F, B, C, E, D);	_("CFABED", C, F, A, B, E, D);	_("EFABDC", E, F, A, B, D, C);
			_("AFBDCE", A, F, B, D, C, E);	_("CFADBE", C, F, A, D, B, E);	_("EFACBD", E, F, A, C, B, D);
			_("AFBDEC", A, F, B, D, E, C);	_("CFADEB", C, F, A, D, E, B);	_("EFACDB", E, F, A, C, D, B);
			_("AFBECD", A, F, B, E, C, D);	_("CFAEBD", C, F, A, E, B, D);	_("EFADBC", E, F, A, D, B, C);
			_("AFBEDC", A, F, B, E, D, C);	_("CFAEDB", C, F, A, E, D, B);	_("EFADCB", E, F, A, D, C, B);
			_("AFCBDE", A, F, C, B, D, E);	_("CFBADE", C, F, B, A, D, E);	_("EFBACD", E, F, B, A, C, D);
			_("AFCBED", A, F, C, B, E, D);	_("CFBAED", C, F, B, A, E, D);	_("EFBADC", E, F, B, A, D, C);
			_("AFCDBE", A, F, C, D, B, E);	_("CFBDAE", C, F, B, D, A, E);	_("EFBCAD", E, F, B, C, A, D);
			_("AFCDEB", A, F, C, D, E, B);	_("CFBDEA", C, F, B, D, E, A);	_("EFBCDA", E, F, B, C, D, A);
			_("AFCEBD", A, F, C, E, B, D);	_("CFBEAD", C, F, B, E, A, D);	_("EFBDAC", E, F, B, D, A, C);
			_("AFCEDB", A, F, C, E, D, B);	_("CFBEDA", C, F, B, E, D, A);	_("EFBDCA", E, F, B, D, C, A);
			_("AFDBCE", A, F, D, B, C, E);	_("CFDABE", C, F, D, A, B, E);	_("EFCABD", E, F, C, A, B, D);
			_("AFDBEC", A, F, D, B, E, C);	_("CFDAEB", C, F, D, A, E, B);	_("EFCADB", E, F, C, A, D, B);
			_("AFDCBE", A, F, D, C, B, E);	_("CFDBAE", C, F, D, B, A, E);	_("EFCBAD", E, F, C, B, A, D);
			_("AFDCEB", A, F, D, C, E, B);	_("CFDBEA", C, F, D, B, E, A);	_("EFCBDA", E, F, C, B, D, A);
			_("AFDEBC", A, F, D, E, B, C);	_("CFDEAB", C, F, D, E, A, B);	_("EFCDAB", E, F, C, D, A, B);
			_("AFDECB", A, F, D, E, C, B);	_("CFDEBA", C, F, D, E, B, A);	_("EFCDBA", E, F, C, D, B, A);
			_("AFEBCD", A, F, E, B, C, D);	_("CFEABD", C, F, E, A, B, D);	_("EFDABC", E, F, D, A, B, C);
			_("AFEBDC", A, F, E, B, D, C);	_("CFEADB", C, F, E, A, D, B);	_("EFDACB", E, F, D, A, C, B);
			_("AFECBD", A, F, E, C, B, D);	_("CFEBAD", C, F, E, B, A, D);	_("EFDBAC", E, F, D, B, A, C);
			_("AFECDB", A, F, E, C, D, B);	_("CFEBDA", C, F, E, B, D, A);	_("EFDBCA", E, F, D, B, C, A);
			_("AFEDBC", A, F, E, D, B, C);	_("CFEDAB", C, F, E, D, A, B);	_("EFDCAB", E, F, D, C, A, B);
			_("AFEDCB", A, F, E, D, C, B);	_("CFEDBA", C, F, E, D, B, A);	_("EFDCBA", E, F, D, C, B, A);

			_("BACDEF", B, A, C, D, E, F);	_("DABCEF", D, A, B, C, E, F);	_("FABCDE", F, A, B, C, D, E);
			_("BACDFE", B, A, C, D, F, E);	_("DABCFE", D, A, B, C, F, E);	_("FABCED", F, A, B, C, E, D);
			_("BACEDF", B, A, C, E, D, F);	_("DABECF", D, A, B, E, C, F);	_("FABDCE", F, A, B, D, C, E);
			_("BACEFD", B, A, C, E, F, D);	_("DABEFC", D, A, B, E, F, C);	_("FABDEC", F, A, B, D, E, C);
			_("BACFDE", B, A, C, F, D, E);	_("DABFCE", D, A, B, F, C, E);	_("FABECD", F, A, B, E, C, D);
			_("BACFED", B, A, C, F, E, D);	_("DABFEC", D, A, B, F, E, C);	_("FABEDC", F, A, B, E, D, C);
			_("BADCEF", B, A, D, C, E, F);	_("DACBEF", D, A, C, B, E, F);	_("FACBDE", F, A, C, B, D, E);
			_("BADCFE", B, A, D, C, F, E);	_("DACBFE", D, A, C, B, F, E);	_("FACBED", F, A, C, B, E, D);
			_("BADECF", B, A, D, E, C, F);	_("DACEBF", D, A, C, E, B, F);	_("FACDBE", F, A, C, D, B, E);
			_("BADEFC", B, A, D, E, F, C);	_("DACEFB", D, A, C, E, F, B);	_("FACDEB", F, A, C, D, E, B);
			_("BADFCE", B, A, D, F, C, E);	_("DACFBE", D, A, C, F, B, E);	_("FACEBD", F, A, C, E, B, D);
			_("BADFEC", B, A, D, F, E, C);	_("DACFEB", D, A, C, F, E, B);	_("FACEDB", F, A, C, E, D, B);
			_("BAECDF", B, A, E, C, D, F);	_("DAEBCF", D, A, E, B, C, F);	_("FADBCE", F, A, D, B, C, E);
			_("BAECFD", B, A, E, C, F, D);	_("DAEBFC", D, A, E, B, F, C);	_("FADBEC", F, A, D, B, E, C);
			_("BAEDCF", B, A, E, D, C, F);	_("DAECBF", D, A, E, C, B, F);	_("FADCBE", F, A, D, C, B, E);
			_("BAEDFC", B, A, E, D, F, C);	_("DAECFB", D, A, E, C, F, B);	_("FADCEB", F, A, D, C, E, B);
			_("BAEFCD", B, A, E, F, C, D);	_("DAEFBC", D, A, E, F, B, C);	_("FADEBC", F, A, D, E, B, C);
			_("BAEFDC", B, A, E, F, D, C);	_("DAEFCB", D, A, E, F, C, B);	_("FADECB", F, A, D, E, C, B);
			_("BAFCDE", B, A, F, C, D, E);	_("DAFBCE", D, A, F, B, C, E);	_("FAEBCD", F, A, E, B, C, D);
			_("BAFCED", B, A, F, C, E, D);	_("DAFBEC", D, A, F, B, E, C);	_("FAEBDC", F, A, E, B, D, C);
			_("BAFDCE", B, A, F, D, C, E);	_("DAFCBE", D, A, F, C, B, E);	_("FAECBD", F, A, E, C, B, D);
			_("BAFDEC", B, A, F, D, E, C);	_("DAFCEB", D, A, F, C, E, B);	_("FAECDB", F, A, E, C, D, B);
			_("BAFECD", B, A, F, E, C, D);	_("DAFEBC", D, A, F, E, B, C);	_("FAEDBC", F, A, E, D, B, C);
			_("BAFEDC", B, A, F, E, D, C);	_("DAFECB", D, A, F, E, C, B);	_("FAEDCB", F, A, E, D, C, B);
			_("BCADEF", B, C, A, D, E, F);	_("DBACEF", D, B, A, C, E, F);	_("FBACDE", F, B, A, C, D, E);
			_("BCADFE", B, C, A, D, F, E);	_("DBACFE", D, B, A, C, F, E);	_("FBACED", F, B, A, C, E, D);
			_("BCAEDF", B, C, A, E, D, F);	_("DBAECF", D, B, A, E, C, F);	_("FBADCE", F, B, A, D, C, E);
			_("BCAEFD", B, C, A, E, F, D);	_("DBAEFC", D, B, A, E, F, C);	_("FBADEC", F, B, A, D, E, C);
			_("BCAFDE", B, C, A, F, D, E);	_("DBAFCE", D, B, A, F, C, E);	_("FBAECD", F, B, A, E, C, D);
			_("BCAFED", B, C, A, F, E, D);	_("DBAFEC", D, B, A, F, E, C);	_("FBAEDC", F, B, A, E, D, C);
			_("BCDAEF", B, C, D, A, E, F);	_("DBCAEF", D, B, C, A, E, F);	_("FBCADE", F, B, C, A, D, E);
			_("BCDAFE", B, C, D, A, F, E);	_("DBCAFE", D, B, C, A, F, E);	_("FBCAED", F, B, C, A, E, D);
			_("BCDEAF", B, C, D, E, A, F);	_("DBCEAF", D, B, C, E, A, F);	_("FBCDAE", F, B, C, D, A, E);
			_("BCDEFA", B, C, D, E, F, A);	_("DBCEFA", D, B, C, E, F, A);	_("FBCDEA", F, B, C, D, E, A);
			_("BCDFAE", B, C, D, F, A, E);	_("DBCFAE", D, B, C, F, A, E);	_("FBCEAD", F, B, C, E, A, D);
			_("BCDFEA", B, C, D, F, E, A);	_("DBCFEA", D, B, C, F, E, A);	_("FBCEDA", F, B, C, E, D, A);
			_("BCEADF", B, C, E, A, D, F);	_("DBEACF", D, B, E, A, C, F);	_("FBDACE", F, B, D, A, C, E);
			_("BCEAFD", B, C, E, A, F, D);	_("DBEAFC", D, B, E, A, F, C);	_("FBDAEC", F, B, D, A, E, C);
			_("BCEDAF", B, C, E, D, A, F);	_("DBECAF", D, B, E, C, A, F);	_("FBDCAE", F, B, D, C, A, E);
			_("BCEDFA", B, C, E, D, F, A);	_("DBECFA", D, B, E, C, F, A);	_("FBDCEA", F, B, D, C, E, A);
			_("BCEFAD", B, C, E, F, A, D);	_("DBEFAC", D, B, E, F, A, C);	_("FBDEAC", F, B, D, E, A, C);
			_("BCEFDA", B, C, E, F, D, A);	_("DBEFCA", D, B, E, F, C, A);	_("FBDECA", F, B, D, E, C, A);
			_("BCFADE", B, C, F, A, D, E);	_("DBFACE", D, B, F, A, C, E);	_("FBEACD", F, B, E, A, C, D);
			_("BCFAED", B, C, F, A, E, D);	_("DBFAEC", D, B, F, A, E, C);	_("FBEADC", F, B, E, A, D, C);
			_("BCFDAE", B, C, F, D, A, E);	_("DBFCAE", D, B, F, C, A, E);	_("FBECAD", F, B, E, C, A, D);
			_("BCFDEA", B, C, F, D, E, A);	_("DBFCEA", D, B, F, C, E, A);	_("FBECDA", F, B, E, C, D, A);
			_("BCFEAD", B, C, F, E, A, D);	_("DBFEAC", D, B, F, E, A, C);	_("FBEDAC", F, B, E, D, A, C);
			_("BCFEDA", B, C, F, E, D, A);	_("DBFECA", D, B, F, E, C, A);	_("FBEDCA", F, B, E, D, C, A);
			_("BDACEF", B, D, A, C, E, F);	_("DCABEF", D, C, A, B, E, F);	_("FCABDE", F, C, A, B, D, E);
			_("BDACFE", B, D, A, C, F, E);	_("DCABFE", D, C, A, B, F, E);	_("FCABED", F, C, A, B, E, D);
			_("BDAECF", B, D, A, E, C, F);	_("DCAEBF", D, C, A, E, B, F);	_("FCADBE", F, C, A, D, B, E);
			_("BDAEFC", B, D, A, E, F, C);	_("DCAEFB", D, C, A, E, F, B);	_("FCADEB", F, C, A, D, E, B);
			_("BDAFCE", B, D, A, F, C, E);	_("DCAFBE", D, C, A, F, B, E);	_("FCAEBD", F, C, A, E, B, D);
			_("BDAFEC", B, D, A, F, E, C);	_("DCAFEB", D, C, A, F, E, B);	_("FCAEDB", F, C, A, E, D, B);
			_("BDCAEF", B, D, C, A, E, F);	_("DCBAEF", D, C, B, A, E, F);	_("FCBADE", F, C, B, A, D, E);
			_("BDCAFE", B, D, C, A, F, E);	_("DCBAFE", D, C, B, A, F, E);	_("FCBAED", F, C, B, A, E, D);
			_("BDCEAF", B, D, C, E, A, F);	_("DCBEAF", D, C, B, E, A, F);	_("FCBDAE", F, C, B, D, A, E);
			_("BDCEFA", B, D, C, E, F, A);	_("DCBEFA", D, C, B, E, F, A);	_("FCBDEA", F, C, B, D, E, A);
			_("BDCFAE", B, D, C, F, A, E);	_("DCBFAE", D, C, B, F, A, E);	_("FCBEAD", F, C, B, E, A, D);
			_("BDCFEA", B, D, C, F, E, A);	_("DCBFEA", D, C, B, F, E, A);	_("FCBEDA", F, C, B, E, D, A);
			_("BDEACF", B, D, E, A, C, F);	_("DCEABF", D, C, E, A, B, F);	_("FCDABE", F, C, D, A, B, E);
			_("BDEAFC", B, D, E, A, F, C);	_("DCEAFB", D, C, E, A, F, B);	_("FCDAEB", F, C, D, A, E, B);
			_("BDECAF", B, D, E, C, A, F);	_("DCEBAF", D, C, E, B, A, F);	_("FCDBAE", F, C, D, B, A, E);
			_("BDECFA", B, D, E, C, F, A);	_("DCEBFA", D, C, E, B, F, A);	_("FCDBEA", F, C, D, B, E, A);
			_("BDEFAC", B, D, E, F, A, C);	_("DCEFAB", D, C, E, F, A, B);	_("FCDEAB", F, C, D, E, A, B);
			_("BDEFCA", B, D, E, F, C, A);	_("DCEFBA", D, C, E, F, B, A);	_("FCDEBA", F, C, D, E, B, A);
			_("BDFACE", B, D, F, A, C, E);	_("DCFABE", D, C, F, A, B, E);	_("FCEABD", F, C, E, A, B, D);
			_("BDFAEC", B, D, F, A, E, C);	_("DCFAEB", D, C, F, A, E, B);	_("FCEADB", F, C, E, A, D, B);
			_("BDFCAE", B, D, F, C, A, E);	_("DCFBAE", D, C, F, B, A, E);	_("FCEBAD", F, C, E, B, A, D);
			_("BDFCEA", B, D, F, C, E, A);	_("DCFBEA", D, C, F, B, E, A);	_("FCEBDA", F, C, E, B, D, A);
			_("BDFEAC", B, D, F, E, A, C);	_("DCFEAB", D, C, F, E, A, B);	_("FCEDAB", F, C, E, D, A, B);
			_("BDFECA", B, D, F, E, C, A);	_("DCFEBA", D, C, F, E, B, A);	_("FCEDBA", F, C, E, D, B, A);
			_("BEACDF", B, E, A, C, D, F);	_("DEABCF", D, E, A, B, C, F);	_("FDABCE", F, D, A, B, C, E);
			_("BEACFD", B, E, A, C, F, D);	_("DEABFC", D, E, A, B, F, C);	_("FDABEC", F, D, A, B, E, C);
			_("BEADCF", B, E, A, D, C, F);	_("DEACBF", D, E, A, C, B, F);	_("FDACBE", F, D, A, C, B, E);
			_("BEADFC", B, E, A, D, F, C);	_("DEACFB", D, E, A, C, F, B);	_("FDACEB", F, D, A, C, E, B);
			_("BEAFCD", B, E, A, F, C, D);	_("DEAFBC", D, E, A, F, B, C);	_("FDAEBC", F, D, A, E, B, C);
			_("BEAFDC", B, E, A, F, D, C);	_("DEAFCB", D, E, A, F, C, B);	_("FDAECB", F, D, A, E, C, B);
			_("BECADF", B, E, C, A, D, F);	_("DEBACF", D, E, B, A, C, F);	_("FDBACE", F, D, B, A, C, E);
			_("BECAFD", B, E, C, A, F, D);	_("DEBAFC", D, E, B, A, F, C);	_("FDBAEC", F, D, B, A, E, C);
			_("BECDAF", B, E, C, D, A, F);	_("DEBCAF", D, E, B, C, A, F);	_("FDBCAE", F, D, B, C, A, E);
			_("BECDFA", B, E, C, D, F, A);	_("DEBCFA", D, E, B, C, F, A);	_("FDBCEA", F, D, B, C, E, A);
			_("BECFAD", B, E, C, F, A, D);	_("DEBFAC", D, E, B, F, A, C);	_("FDBEAC", F, D, B, E, A, C);
			_("BECFDA", B, E, C, F, D, A);	_("DEBFCA", D, E, B, F, C, A);	_("FDBECA", F, D, B, E, C, A);
			_("BEDACF", B, E, D, A, C, F);	_("DECABF", D, E, C, A, B, F);	_("FDCABE", F, D, C, A, B, E);
			_("BEDAFC", B, E, D, A, F, C);	_("DECAFB", D, E, C, A, F, B);	_("FDCAEB", F, D, C, A, E, B);
			_("BEDCAF", B, E, D, C, A, F);	_("DECBAF", D, E, C, B, A, F);	_("FDCBAE", F, D, C, B, A, E);
			_("BEDCFA", B, E, D, C, F, A);	_("DECBFA", D, E, C, B, F, A);	_("FDCBEA", F, D, C, B, E, A);
			_("BEDFAC", B, E, D, F, A, C);	_("DECFAB", D, E, C, F, A, B);	_("FDCEAB", F, D, C, E, A, B);
			_("BEDFCA", B, E, D, F, C, A);	_("DECFBA", D, E, C, F, B, A);	_("FDCEBA", F, D, C, E, B, A);
			_("BEFACD", B, E, F, A, C, D);	_("DEFABC", D, E, F, A, B, C);	_("FDEABC", F, D, E, A, B, C);
			_("BEFADC", B, E, F, A, D, C);	_("DEFACB", D, E, F, A, C, B);	_("FDEACB", F, D, E, A, C, B);
			_("BEFCAD", B, E, F, C, A, D);	_("DEFBAC", D, E, F, B, A, C);	_("FDEBAC", F, D, E, B, A, C);
			_("BEFCDA", B, E, F, C, D, A);	_("DEFBCA", D, E, F, B, C, A);	_("FDEBCA", F, D, E, B, C, A);
			_("BEFDAC", B, E, F, D, A, C);	_("DEFCAB", D, E, F, C, A, B);	_("FDECAB", F, D, E, C, A, B);
			_("BEFDCA", B, E, F, D, C, A);	_("DEFCBA", D, E, F, C, B, A);	_("FDECBA", F, D, E, C, B, A);
			_("BFACDE", B, F, A, C, D, E);	_("DFABCE", D, F, A, B, C, E);	_("FEABCD", F, E, A, B, C, D);
			_("BFACED", B, F, A, C, E, D);	_("DFABEC", D, F, A, B, E, C);	_("FEABDC", F, E, A, B, D, C);
			_("BFADCE", B, F, A, D, C, E);	_("DFACBE", D, F, A, C, B, E);	_("FEACBD", F, E, A, C, B, D);
			_("BFADEC", B, F, A, D, E, C);	_("DFACEB", D, F, A, C, E, B);	_("FEACDB", F, E, A, C, D, B);
			_("BFAECD", B, F, A, E, C, D);	_("DFAEBC", D, F, A, E, B, C);	_("FEADBC", F, E, A, D, B, C);
			_("BFAEDC", B, F, A, E, D, C);	_("DFAECB", D, F, A, E, C, B);	_("FEADCB", F, E, A, D, C, B);
			_("BFCADE", B, F, C, A, D, E);	_("DFBACE", D, F, B, A, C, E);	_("FEBACD", F, E, B, A, C, D);
			_("BFCAED", B, F, C, A, E, D);	_("DFBAEC", D, F, B, A, E, C);	_("FEBADC", F, E, B, A, D, C);
			_("BFCDAE", B, F, C, D, A, E);	_("DFBCAE", D, F, B, C, A, E);	_("FEBCAD", F, E, B, C, A, D);
			_("BFCDEA", B, F, C, D, E, A);	_("DFBCEA", D, F, B, C, E, A);	_("FEBCDA", F, E, B, C, D, A);
			_("BFCEAD", B, F, C, E, A, D);	_("DFBEAC", D, F, B, E, A, C);	_("FEBDAC", F, E, B, D, A, C);
			_("BFCEDA", B, F, C, E, D, A);	_("DFBECA", D, F, B, E, C, A);	_("FEBDCA", F, E, B, D, C, A);
			_("BFDACE", B, F, D, A, C, E);	_("DFCABE", D, F, C, A, B, E);	_("FECABD", F, E, C, A, B, D);
			_("BFDAEC", B, F, D, A, E, C);	_("DFCAEB", D, F, C, A, E, B);	_("FECADB", F, E, C, A, D, B);
			_("BFDCAE", B, F, D, C, A, E);	_("DFCBAE", D, F, C, B, A, E);	_("FECBAD", F, E, C, B, A, D);
			_("BFDCEA", B, F, D, C, E, A);	_("DFCBEA", D, F, C, B, E, A);	_("FECBDA", F, E, C, B, D, A);
			_("BFDEAC", B, F, D, E, A, C);	_("DFCEAB", D, F, C, E, A, B);	_("FECDAB", F, E, C, D, A, B);
			_("BFDECA", B, F, D, E, C, A);	_("DFCEBA", D, F, C, E, B, A);	_("FECDBA", F, E, C, D, B, A);
			_("BFEACD", B, F, E, A, C, D);	_("DFEABC", D, F, E, A, B, C);	_("FEDABC", F, E, D, A, B, C);
			_("BFEADC", B, F, E, A, D, C);	_("DFEACB", D, F, E, A, C, B);	_("FEDACB", F, E, D, A, C, B);
			_("BFECAD", B, F, E, C, A, D);	_("DFEBAC", D, F, E, B, A, C);	_("FEDBAC", F, E, D, B, A, C);
			_("BFECDA", B, F, E, C, D, A);	_("DFEBCA", D, F, E, B, C, A);	_("FEDBCA", F, E, D, B, C, A);
			_("BFEDAC", B, F, E, D, A, C);	_("DFECAB", D, F, E, C, A, B);	_("FEDCAB", F, E, D, C, A, B);
			_("BFEDCA", B, F, E, D, C, A);	_("DFECBA", D, F, E, C, B, A);	_("FEDCBA", F, E, D, C, B, A);
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

