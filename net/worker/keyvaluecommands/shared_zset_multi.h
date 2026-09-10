#ifndef SHARED_ZSET_MULTI_H_
#define SHARED_ZSET_MULTI_H_

#include "mystring.h"
#include "stringtokenizer.h"
#include "pair.h"

/*
Reverse Set a la Redis ZSET

- One to One set
- Manual keySort



Permutation<1 to 6>:

keyN~~keySub			-> index~keySort
keyN~A~index~keySort~keySub	-> keySub



Permutation1NoIndex used in Morton and Geo (with IndexController)

keyN~~keySub			-> index~keySort
keyN~index~keySort~keySub	-> value

*/

namespace net::worker::shared::zsetmulti{

	struct IZSetMultyFactory : hm4::PairFactory::IFactory{
		virtual std::string_view getIndex() const = 0;
	};

	struct Permutation1NoIndex;

	namespace impl_{
		template<size_t N>
		bool valid(std::array<std::string_view, N> const &indexes){
			for(auto const &x : indexes)
				if (x.empty())
					return false;

			return true;
		}

		template<typename Permutation, typename IndexController>
		std::string_view encodeIndex(hm4::PairBufferKey &bufferVal, std::string_view separator, std::array<std::string_view, Permutation::N> const &indexes,
							std::string_view value){
			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				return Permutation::encodeIndex(bufferVal, separator, indexes);
			}else{
				using P1    = Permutation1NoIndex;
				static_assert(std::is_same_v<Permutation, P1>, "This works only with Permutation1NoIndex");

				logger<Logger::DEBUG>() << "Using IndexController to encode";
				return IndexController::encode(value);
			}
		}

		template<typename Permutation, typename IndexController>
		std::array<std::string_view, Permutation::N> decodeIndex(std::string_view separator, std::string_view value){
			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				return Permutation::decodeIndex(separator, value);
			}else{
				using P1    = Permutation1NoIndex;
				static_assert(std::is_same_v<Permutation, P1>, "This works only with Permutation1NoIndex");

				logger<Logger::DEBUG>() << "Using IndexController to decode";
				return IndexController::template decode<Permutation::N>(value);
			}
		}

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
			return hm4::Pair::isCompositeKeyValid(2 * 1 + 0 * 1 + more, keyN, keySub,
						indexes[0]);
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
			return 1 * 1 + keyN.size();
		}

		template<bool LAST_SEPARATOR = true>
		static std::string_view makeKeyRangeN(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
				//	std::string_view /* txt */,
					std::string_view a = ""){

			// things we do for reading + performance :)
			if constexpr(LAST_SEPARATOR){
				if (a.empty())
					return concatenateBuffer(bufferKey,
							key	,	separator
					);

				if constexpr(1)
					return concatenateBuffer(bufferKey,
							key	,	separator	,
							a	,	separator
					);
			}else{
				if (a.empty())
					return concatenateBuffer(bufferKey,
							key	,	separator
					);

				if constexpr(1)
					return concatenateBuffer(bufferKey,
							key	,	separator	,
							a		// missing separator
					);
			}
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::string_view ix0
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
						ix0	,	separator	,
					keySub
			);
		}

		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub, indexes[0]);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A] = indexes;

			auto _ = [&](std::string_view a){
				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, a);

				func(key);
			};

			_(A);
		}
	};



	template<int>
	struct Permutation;



	template<>
	struct Permutation<1>{
		constexpr static size_t N = 1 + 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~A~a~keySort~keySub, (N + 2) * ~ + N -> {A}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~A~a~keySort~keySub, (N + 2) * ~ + N -> {A}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub,
						indexes[0],
						indexes[1]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
			// no need to copy, but lets do it anyway, because the caller expects it.
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

		static std::string_view makeKeyRange(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = ""){

			if (a.empty())
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			if constexpr(1)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
							std::string_view ix0,
							std::string_view ixS
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
					txt	,		separator	,
						ix0	,	separator	,
						ixS	,	separator	,
					keySub
			);
		}


		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub,
								"A",
									indexes[0],
									indexes[1]
			);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, S] = indexes;

			auto _ = [&](std::string_view txt, std::string_view a){
				auto const [A, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, txt, a, S);

				func(key);
			};

			_("A", A);
		}
	};



	template<>
	struct Permutation<2>{
		constexpr static size_t N = 2 + 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~AB~a~b~keySort~keySub, (N + 2) * ~ + N -> {AB}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~AB~a~b~keySort~keySub, (N + 2) * ~ + N -> {AB}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub,
						indexes[0],
						indexes[1],
						indexes[2]);
		}

		static auto encodeIndex(hm4::PairBufferKey &bufferKey, std::string_view separator, std::array<std::string_view, N> const &indexes){
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

		static std::string_view makeKeyRange(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = ""){

			if (a.empty())
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			if (b.empty() || txt.size() < 2)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			if constexpr(1)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
							std::string_view ix0,
							std::string_view ix1,
							std::string_view ixS
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
					txt	,		separator	,
						ix0	,	separator	,
						ix1	,	separator	,
						ixS	,	separator	,
					keySub
			);
		}


		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub,
								"AB",
									indexes[0],
									indexes[1],
									indexes[2]
			);
		}


		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, S] = indexes;

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b){
				auto const [A, B, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, txt, a, b, S);

				func(key);
			};

			std::string_view const o{};

			_("A",  A, o);
			_("B",  B, o);

			_("AB", A, B);
			_("BA", B, A);
		}
	};



	template<>
	struct Permutation<3>{
		constexpr static size_t N = 3 + 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABC~A~B~C~keySort~keySub, (N + 2) * ~ + N {ABC}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABC~A~B~C~keySort~keySub, (N + 2) * ~ + N {ABC}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub,
						indexes[0],
						indexes[1],
						indexes[2],
						indexes[3]);
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

		static std::string_view makeKeyRange(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = ""){

			if (a.empty())
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			if (b.empty() || txt.size() < 2)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			if (c.empty() || txt.size() < 3)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			if constexpr(1)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
							std::string_view ix0,
							std::string_view ix1,
							std::string_view ix2,
							std::string_view ixS
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
					txt	,		separator	,
						ix0	,	separator	,
						ix1	,	separator	,
						ix2	,	separator	,
						ixS	,	separator	,
					keySub
			);
		}


		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub,
								"ABC",
									indexes[0],
									indexes[1],
									indexes[2],
									indexes[3]
			);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, S] = indexes;

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c){
				auto const [A, B, C, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, txt, a, b, c, S);

				func(key);
			};

			std::string_view const o{};

			_("A",   A, o, o);
			_("B",   B, o, o);
			_("C",   C, o, o);

			_("AB",  A, B, o);
			_("BA",  B, A, o);
			_("AC",  A, C, o);
			_("CA",  C, A, o);
			_("BC",  B, C, o);
			_("CB",  C, B, o);

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
		constexpr static size_t N = 4 + 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABCD~A~B~C~D~keySort~keySub, (N + 2) * ~ + N {ABCD}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABCD~A~B~C~D~keySort~keySub, (N + 2) * ~ + N {ABCD}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub,
						indexes[0],
						indexes[1],
						indexes[2],
						indexes[3],
						indexes[4]);
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

		static std::string_view makeKeyRange(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "", std::string_view d = ""){

			if (a.empty())
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			if (b.empty() || txt.size() < 2)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			if (c.empty() || txt.size() < 3)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			if (d.empty() || txt.size() < 4)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			if constexpr(1)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator
				);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
							std::string_view ix0,
							std::string_view ix1,
							std::string_view ix2,
							std::string_view ix3,
							std::string_view ixS
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
					txt	,		separator	,
						ix0	,	separator	,
						ix1	,	separator	,
						ix2	,	separator	,
						ix3	,	separator	,
						ixS	,	separator	,
					keySub
			);
		}


		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub,
								"ABCD",
									indexes[0],
									indexes[1],
									indexes[2],
									indexes[3],
									indexes[4]
			);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, D, S] = indexes;

			auto _ = [&](std::string_view txt, std::string_view a, std::string_view b, std::string_view c, std::string_view d){
				auto const [A, B, C, D, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, txt, a, b, c, d, S);

				func(key);
			};

			std::string_view const o{};

			_("A",    A, o, o, o);
			_("B",    B, o, o, o);
			_("C",    C, o, o, o);
			_("D",    D, o, o, o);

			_("AB",   A, B, o, o);
			_("BA",   B, A, o, o);
			_("AC",   A, C, o, o);
			_("CA",   C, A, o, o);
			_("AD",   A, D, o, o);
			_("DA",   D, A, o, o);
			_("BC",   B, C, o, o);
			_("CB",   C, B, o, o);
			_("BD",   B, D, o, o);
			_("DB",   D, B, o, o);
			_("CD",   C, D, o, o);
			_("DC",   D, C, o, o);

			_("ABC",  A, B, C, o);
			_("ACB",  A, C, B, o);
			_("BAC",  B, A, C, o);
			_("BCA",  B, C, A, o);
			_("CAB",  C, A, B, o);
			_("CBA",  C, B, A, o);
			_("ABD",  A, B, D, o);
			_("ADB",  A, D, B, o);
			_("BAD",  B, A, D, o);
			_("BDA",  B, D, A, o);
			_("DAB",  D, A, B, o);
			_("DBA",  D, B, A, o);
			_("ACD",  A, C, D, o);
			_("ADC",  A, D, C, o);
			_("CAD",  C, A, D, o);
			_("CDA",  C, D, A, o);
			_("DAC",  D, A, C, o);
			_("DCA",  D, C, A, o);
			_("BCD",  B, C, D, o);
			_("BDC",  B, D, C, o);
			_("CBD",  C, B, D, o);
			_("CDB",  C, D, B, o);
			_("DBC",  D, B, C, o);
			_("DCB",  D, C, B, o);

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
		constexpr static size_t N = 5 + 1;

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, size_t more = 0){
			// keyN~ABCDE~A~B~C~D~E~keySort~keySub, (N + 2) * ~ + N {ABCDE}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub);
		}

		constexpr static bool valid(std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, size_t more = 0){
			// keyN~ABCDE~A~B~C~D~E~keySort~keySub, (N + 2) * ~ + N {ABCDE}
			return hm4::Pair::isCompositeKeyValid((N + 2) + N + more, keyN, keySub,
						indexes[0],
						indexes[1],
						indexes[2],
						indexes[3],
						indexes[4],
						indexes[5]);
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

		static std::string_view makeKeyRange(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view key,
					std::string_view txt,
					std::string_view a = "", std::string_view b = "", std::string_view c = "",
					std::string_view d = "", std::string_view e = ""){

			if (a.empty())
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator
				);

			if (b.empty() || txt.size() < 2)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator
				);

			if (c.empty() || txt.size() < 3)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator
				);

			if (d.empty() || txt.size() < 4)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator
				);

			if (e.empty() || txt.size() < 5)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator
				);

			if constexpr(1)
				return concatenateBuffer(bufferKey,
						key	,	separator	,
						txt	,	separator	,
						a	,	separator	,
						b	,	separator	,
						c	,	separator	,
						d	,	separator	,
						e	,	separator
				);
		}

		static std::string_view makeKeyData(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
						std::string_view txt,
							std::string_view ix0,
							std::string_view ix1,
							std::string_view ix2,
							std::string_view ix3,
							std::string_view ix4,
							std::string_view ixS
				){

			return concatenateBuffer(bufferKey,
					keyN	,		separator	,
					txt	,		separator	,
						ix0	,	separator	,
						ix1	,	separator	,
						ix2	,	separator	,
						ix3	,	separator	,
						ix4	,	separator	,
						ixS	,	separator	,
					keySub
			);
		}


		static std::string_view makeKeyDataFirst(hm4::PairBufferKey &bufferKey, std::string_view separator,
					std::string_view keyN,
					std::string_view keySub,
					std::array<std::string_view, N> const &indexes){

			return makeKeyData(bufferKey, separator, keyN, keySub,
								"ABCDE",
									indexes[0],
									indexes[1],
									indexes[2],
									indexes[3],
									indexes[4],
									indexes[5]
			);
		}

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, D, E, S] = indexes;

			auto _ = [&](std::string_view txt,
							std::string_view a, std::string_view b, std::string_view c,
							std::string_view d, std::string_view e){

				auto const [A, B, C, D, E, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = makeKeyData(bufferKey, separator, keyN, keySub, txt, a, b, c, d, e, S);

				func(key);
			};

			// 1 =>    5
			// 2 =>   20
			// 3 =>   60
			// 4 =>  120
			// 5 =>  120
			// Total 325 permutations total

			std::string_view const o{};

			_("A",     A, o, o, o, o);
			_("B",     B, o, o, o, o);
			_("C",     C, o, o, o, o);
			_("D",     D, o, o, o, o);
			_("E",     E, o, o, o, o);

			_("AB",    A, B, o, o, o);
			_("BA",    B, A, o, o, o);
			_("AC",    A, C, o, o, o);
			_("CA",    C, A, o, o, o);
			_("AD",    A, D, o, o, o);
			_("DA",    D, A, o, o, o);
			_("AE",    A, E, o, o, o);
			_("EA",    E, A, o, o, o);
			_("BC",    B, C, o, o, o);
			_("CB",    C, B, o, o, o);
			_("BD",    B, D, o, o, o);
			_("DB",    D, B, o, o, o);
			_("BE",    B, E, o, o, o);
			_("EB",    E, B, o, o, o);
			_("CD",    C, D, o, o, o);
			_("DC",    D, C, o, o, o);
			_("CE",    C, E, o, o, o);
			_("EC",    E, C, o, o, o);
			_("DE",    D, E, o, o, o);
			_("ED",    E, D, o, o, o);

			_("ABC",   A, B, C, o, o);
			_("ACB",   A, C, B, o, o);
			_("BAC",   B, A, C, o, o);
			_("BCA",   B, C, A, o, o);
			_("CAB",   C, A, B, o, o);
			_("CBA",   C, B, A, o, o);
			_("ABD",   A, B, D, o, o);
			_("ADB",   A, D, B, o, o);
			_("BAD",   B, A, D, o, o);
			_("BDA",   B, D, A, o, o);
			_("DAB",   D, A, B, o, o);
			_("DBA",   D, B, A, o, o);
			_("ABE",   A, B, E, o, o);
			_("AEB",   A, E, B, o, o);
			_("BAE",   B, A, E, o, o);
			_("BEA",   B, E, A, o, o);
			_("EAB",   E, A, B, o, o);
			_("EBA",   E, B, A, o, o);
			_("ACD",   A, C, D, o, o);
			_("ADC",   A, D, C, o, o);
			_("CAD",   C, A, D, o, o);
			_("CDA",   C, D, A, o, o);
			_("DAC",   D, A, C, o, o);
			_("DCA",   D, C, A, o, o);
			_("ACE",   A, C, E, o, o);
			_("AEC",   A, E, C, o, o);
			_("CAE",   C, A, E, o, o);
			_("CEA",   C, E, A, o, o);
			_("EAC",   E, A, C, o, o);
			_("ECA",   E, C, A, o, o);
			_("ADE",   A, D, E, o, o);
			_("AED",   A, E, D, o, o);
			_("DAE",   D, A, E, o, o);
			_("DEA",   D, E, A, o, o);
			_("EAD",   E, A, D, o, o);
			_("EDA",   E, D, A, o, o);
			_("BCD",   B, C, D, o, o);
			_("BDC",   B, D, C, o, o);
			_("CBD",   C, B, D, o, o);
			_("CDB",   C, D, B, o, o);
			_("DBC",   D, B, C, o, o);
			_("DCB",   D, C, B, o, o);
			_("BCE",   B, C, E, o, o);
			_("BEC",   B, E, C, o, o);
			_("CBE",   C, B, E, o, o);
			_("CEB",   C, E, B, o, o);
			_("EBC",   E, B, C, o, o);
			_("ECB",   E, C, B, o, o);
			_("BDE",   B, D, E, o, o);
			_("BED",   B, E, D, o, o);
			_("DBE",   D, B, E, o, o);
			_("DEB",   D, E, B, o, o);
			_("EBD",   E, B, D, o, o);
			_("EDB",   E, D, B, o, o);
			_("CDE",   C, D, E, o, o);
			_("CED",   C, E, D, o, o);
			_("DCE",   D, C, E, o, o);
			_("DEC",   D, E, C, o, o);
			_("ECD",   E, C, D, o, o);
			_("EDC",   E, D, C, o, o);

			_("ABCD",  A, B, C, D, o);
			_("ABDC",  A, B, D, C, o);
			_("ACBD",  A, C, B, D, o);
			_("ACDB",  A, C, D, B, o);
			_("ADBC",  A, D, B, C, o);
			_("ADCB",  A, D, C, B, o);
			_("BACD",  B, A, C, D, o);
			_("BADC",  B, A, D, C, o);
			_("BCAD",  B, C, A, D, o);
			_("BCDA",  B, C, D, A, o);
			_("BDAC",  B, D, A, C, o);
			_("BDCA",  B, D, C, A, o);
			_("CABD",  C, A, B, D, o);
			_("CADB",  C, A, D, B, o);
			_("CBAD",  C, B, A, D, o);
			_("CBDA",  C, B, D, A, o);
			_("CDAB",  C, D, A, B, o);
			_("CDBA",  C, D, B, A, o);
			_("DABC",  D, A, B, C, o);
			_("DACB",  D, A, C, B, o);
			_("DBAC",  D, B, A, C, o);
			_("DBCA",  D, B, C, A, o);
			_("DCAB",  D, C, A, B, o);
			_("DCBA",  D, C, B, A, o);
			_("ABCE",  A, B, C, E, o);
			_("ABEC",  A, B, E, C, o);
			_("ACBE",  A, C, B, E, o);
			_("ACEB",  A, C, E, B, o);
			_("AEBC",  A, E, B, C, o);
			_("AECB",  A, E, C, B, o);
			_("BACE",  B, A, C, E, o);
			_("BAEC",  B, A, E, C, o);
			_("BCAE",  B, C, A, E, o);
			_("BCEA",  B, C, E, A, o);
			_("BEAC",  B, E, A, C, o);
			_("BECA",  B, E, C, A, o);
			_("CABE",  C, A, B, E, o);
			_("CAEB",  C, A, E, B, o);
			_("CBAE",  C, B, A, E, o);
			_("CBEA",  C, B, E, A, o);
			_("CEAB",  C, E, A, B, o);
			_("CEBA",  C, E, B, A, o);
			_("EABC",  E, A, B, C, o);
			_("EACB",  E, A, C, B, o);
			_("EBAC",  E, B, A, C, o);
			_("EBCA",  E, B, C, A, o);
			_("ECAB",  E, C, A, B, o);
			_("ECBA",  E, C, B, A, o);
			_("ABDE",  A, B, D, E, o);
			_("ABED",  A, B, E, D, o);
			_("ADBE",  A, D, B, E, o);
			_("ADEB",  A, D, E, B, o);
			_("AEBD",  A, E, B, D, o);
			_("AEDB",  A, E, D, B, o);
			_("BADE",  B, A, D, E, o);
			_("BAED",  B, A, E, D, o);
			_("BDAE",  B, D, A, E, o);
			_("BDEA",  B, D, E, A, o);
			_("BEAD",  B, E, A, D, o);
			_("BEDA",  B, E, D, A, o);
			_("DABE",  D, A, B, E, o);
			_("DAEB",  D, A, E, B, o);
			_("DBAE",  D, B, A, E, o);
			_("DBEA",  D, B, E, A, o);
			_("DEAB",  D, E, A, B, o);
			_("DEBA",  D, E, B, A, o);
			_("EABD",  E, A, B, D, o);
			_("EADB",  E, A, D, B, o);
			_("EBAD",  E, B, A, D, o);
			_("EBDA",  E, B, D, A, o);
			_("EDAB",  E, D, A, B, o);
			_("EDBA",  E, D, B, A, o);
			_("ACDE",  A, C, D, E, o);
			_("ACED",  A, C, E, D, o);
			_("ADCE",  A, D, C, E, o);
			_("ADEC",  A, D, E, C, o);
			_("AECD",  A, E, C, D, o);
			_("AEDC",  A, E, D, C, o);
			_("CADE",  C, A, D, E, o);
			_("CAED",  C, A, E, D, o);
			_("CDAE",  C, D, A, E, o);
			_("CDEA",  C, D, E, A, o);
			_("CEAD",  C, E, A, D, o);
			_("CEDA",  C, E, D, A, o);
			_("DACE",  D, A, C, E, o);
			_("DAEC",  D, A, E, C, o);
			_("DCAE",  D, C, A, E, o);
			_("DCEA",  D, C, E, A, o);
			_("DEAC",  D, E, A, C, o);
			_("DECA",  D, E, C, A, o);
			_("EACD",  E, A, C, D, o);
			_("EADC",  E, A, D, C, o);
			_("ECAD",  E, C, A, D, o);
			_("ECDA",  E, C, D, A, o);
			_("EDAC",  E, D, A, C, o);
			_("EDCA",  E, D, C, A, o);
			_("BCDE",  B, C, D, E, o);
			_("BCED",  B, C, E, D, o);
			_("BDCE",  B, D, C, E, o);
			_("BDEC",  B, D, E, C, o);
			_("BECD",  B, E, C, D, o);
			_("BEDC",  B, E, D, C, o);
			_("CBDE",  C, B, D, E, o);
			_("CBED",  C, B, E, D, o);
			_("CDBE",  C, D, B, E, o);
			_("CDEB",  C, D, E, B, o);
			_("CEBD",  C, E, B, D, o);
			_("CEDB",  C, E, D, B, o);
			_("DBCE",  D, B, C, E, o);
			_("DBEC",  D, B, E, C, o);
			_("DCBE",  D, C, B, E, o);
			_("DCEB",  D, C, E, B, o);
			_("DEBC",  D, E, B, C, o);
			_("DECB",  D, E, C, B, o);
			_("EBCD",  E, B, C, D, o);
			_("EBDC",  E, B, D, C, o);
			_("ECBD",  E, C, B, D, o);
			_("ECDB",  E, C, D, B, o);
			_("EDBC",  E, D, B, C, o);
			_("EDCB",  E, D, C, B, o);

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



	namespace impl_{

		template<typename Permutation, typename DBAdapter>
		void removePreviousIndexes(DBAdapter &db,
				std::string_view keyN, std::string_view keySub, std::array<std::string_view, Permutation::N> const &indexes){

			// make a copy, because values may be invalidated.

			std::array<std::string_view,   Permutation::N> indexesOldSave;
			std::array<hm4::PairBufferKey, Permutation::N> indexesOldBuffer;

			for(size_t i = 0; i < Permutation::N; ++i)
				indexesOldSave[i] = concatenateBuffer(indexesOldBuffer[i], indexes[i]);

			// delete indexes

			auto deleteOldKeys = [&](std::string_view key){
				logger<Logger::DEBUG>() << "ZSetMulti::ADD/REM: DEL old index key" << key;

				erase(*db, key);
			};

			Permutation::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexesOldSave, deleteOldKeys);
		}

		template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
		void removePreviousIndexes(DBAdapter &db,
				std::string_view keyCtrl,
				std::string_view keyN, std::string_view keySub, std::array<std::string_view, Permutation::N> const &indexes){

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

					removePreviousIndexes<Permutation>(db, keyN, keySub, indexesOld);
				}
			}else{
				// Case 2: no ctrl key

				logger<Logger::DEBUG>() << "ZSetMulti::ADD: no ctrl key" << keyCtrl;
			}
		}

	} // namespace impl_



	template<typename Permutation, typename IndexController = std::nullptr_t, typename DBAdapter>
	void add(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, std::array<std::string_view, Permutation::N> const &indexes, std::string_view value){

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: ctrl key" << keyCtrl;

		// Update control key and delete hash key if any
		removePreviousIndexes<Permutation, IndexController>(db, keyCtrl, keyN, keySub, indexes);

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



	template<typename Permutation, typename DBAdapter, typename VPairFactory>
	void addF(DBAdapter &db,
			std::string_view keyN, std::string_view keySub, VPairFactory &factory){

		using P1    = Permutation1NoIndex;
		using VBase = IZSetMultyFactory;

		static_assert(std::is_same_v<Permutation, P1>, "This works only with Permutation1NoIndex");
		static_assert(std::is_base_of_v<VBase, VPairFactory>, "VPairFactory must derive from IZSetMultyFactory");

		using namespace impl_;

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: ctrl key" << keyCtrl;

		// Update control key and delete hash key if any

		auto const index = factory.getIndex();
		std::array<std::string_view, 1> const indexes{ index };

		removePreviousIndexes<P1>(db, keyCtrl, keyN, keySub, indexes );

		auto insertNewKeys = [&](std::string_view key){
			logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET index key" << key;

			factory.setKey(key);

			hm4::PairFactory::IFactory &f = factory;

			insert(*db, f);
		};

		P1::for_each(DBAdapter::SEPARATOR, keyN, keySub, indexes, insertNewKeys);

		logger<Logger::DEBUG>() << "ZSetMulti::ADD: SET ctrl key" << keyCtrl;

		// we are using no IndexController...
		// auto const encodedValue = encodeIndex<Permutation1NoIndex, IndexController>(bufferVal, DBAdapter::SEPARATOR, indexes, value);

		// P1::encodeIndex is just copy of the hash
		// auto const encodedValue = P1::encodeIndex(bufferVal, DBAdapter::SEPARATOR, indexes);

		insert(*db, keyCtrl, index);
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

				removePreviousIndexes<Permutation>(db, keyN, keySub, indexesOld);

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
					auto const keyData = Permutation::makeKeyDataFirst(bufferKeyData, DBAdapter::SEPARATOR, keyN, keySub, indexes);

					logger<Logger::DEBUG>() << "ZSetMulti::GET: data key" << keyData;

					return hm4::getPairVal(*db, keyData);
				}
			}else{
				using P1 = Permutation1NoIndex;
				static_assert(std::is_same_v<Permutation, P1>, "This works only with Permutation1NoIndex");

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

