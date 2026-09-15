#ifndef SHARED_ZSET_MULTI_H_
#define SHARED_ZSET_MULTI_H_

#include "mystring.h"
#include "stringtokenizer.h"
#include "pair.h"

#include "shared_index.h"

/*
Reverse Set a la Redis ZSET

- One to One set
- Manual keySort



Permutation<1 to 5>:

keyN~~keySub			-> index~keySort
keyN~A~index~keySort~keySub	-> keySub



Permutation1NoIndex used in Morton and Geo (with IndexController)

keyN~~keySub			-> index~keySort
keyN~index~keySort~keySub	-> value

*/

namespace net::worker::shared::zset{

	struct IZSetMultyFactory : hm4::PairFactory::IFactory{
		virtual std::string_view getIndex() const = 0;
	};

	struct Permutation1NoIndex;

	namespace impl_{
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

			if (a.empty() || txt[0] == '_')
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, S] = indexes;

			auto _ = [&](std::string_view txt,
						std::string_view a = ""){

				auto const [A, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = concatenateBuffer(bufferKey,
						keyN	,		separator	,
						txt	,		separator	,
							a	,	separator	,
							S	,	separator	,
						keySub
				);

				func(key);
			};

			_("_"	);
			_("A", A);
		}
	};



	template<>
	struct Permutation<2>{
		constexpr static size_t N = 2 + 1;

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

			if (a.empty() || txt[0] == '_')
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, S] = indexes;

			auto _ = [&](std::string_view txt,
						std::string_view a = "",
						std::string_view b = ""){

				auto const [A, B, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = concatenateBuffer(bufferKey,
						keyN	,		separator	,
						txt	,		separator	,
							a	,	separator	,
							b	,	separator	,
							S	,	separator	,
						keySub
				);

				func(key);
			};

			_("_"		);

			_("A",  A	);
			_("B",  B	);

			_("AB", A, B	);
			_("BA", B, A	);
		}
	};



	template<>
	struct Permutation<3>{
		constexpr static size_t N = 3 + 1;

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

			if (a.empty() || txt[0] == '_')
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, S] = indexes;

			auto _ = [&](std::string_view txt,
						std::string_view a = "",
						std::string_view b = "",
						std::string_view c = ""){

				auto const [A, B, C, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = concatenateBuffer(bufferKey,
						keyN	,		separator	,
						txt	,		separator	,
							a	,	separator	,
							b	,	separator	,
							c	,	separator	,
							S	,	separator	,
						keySub
				);

				func(key);
			};

			_("_"		);

			_("A",   A	);
			_("B",   B	);
			_("C",   C	);

			_("AB",  A, B	);
			_("BA",  B, A	);
			_("AC",  A, C	);
			_("CA",  C, A	);
			_("BC",  B, C	);
			_("CB",  C, B	);

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

			if (a.empty() || txt[0] == '_')
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, D, S] = indexes;

			auto _ = [&](std::string_view txt,
						std::string_view a = "",
						std::string_view b = "",
						std::string_view c = "",
						std::string_view d = ""){

				auto const [A, B, C, D, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = concatenateBuffer(bufferKey,
						keyN	,		separator	,
						txt	,		separator	,
							a	,	separator	,
							b	,	separator	,
							c	,	separator	,
							d	,	separator	,
							S	,	separator	,
						keySub
				);

				func(key);
			};

			_("_"			);

			_("A",    A		);
			_("B",    B		);
			_("C",    C		);
			_("D",    D		);

			_("AB",   A, B		);
			_("BA",   B, A		);
			_("AC",   A, C		);
			_("CA",   C, A		);
			_("AD",   A, D		);
			_("DA",   D, A		);
			_("BC",   B, C		);
			_("CB",   C, B		);
			_("BD",   B, D		);
			_("DB",   D, B		);
			_("CD",   C, D		);
			_("DC",   D, C		);

			_("ABC",  A, B, C	);
			_("ACB",  A, C, B	);
			_("BAC",  B, A, C	);
			_("BCA",  B, C, A	);
			_("CAB",  C, A, B	);
			_("CBA",  C, B, A	);
			_("ABD",  A, B, D	);
			_("ADB",  A, D, B	);
			_("BAD",  B, A, D	);
			_("BDA",  B, D, A	);
			_("DAB",  D, A, B	);
			_("DBA",  D, B, A	);
			_("ACD",  A, C, D	);
			_("ADC",  A, D, C	);
			_("CAD",  C, A, D	);
			_("CDA",  C, D, A	);
			_("DAC",  D, A, C	);
			_("DCA",  D, C, A	);
			_("BCD",  B, C, D	);
			_("BDC",  B, D, C	);
			_("CBD",  C, B, D	);
			_("CDB",  C, D, B	);
			_("DBC",  D, B, C	);
			_("DCB",  D, C, B	);

			_("ABCD", A, B, C, D	);
			_("ABDC", A, B, D, C	);
			_("ACBD", A, C, B, D	);
			_("ACDB", A, C, D, B	);
			_("ADBC", A, D, B, C	);
			_("ADCB", A, D, C, B	);
			_("BACD", B, A, C, D	);
			_("BADC", B, A, D, C	);
			_("BCAD", B, C, A, D	);
			_("BCDA", B, C, D, A	);
			_("BDAC", B, D, A, C	);
			_("BDCA", B, D, C, A	);
			_("CABD", C, A, B, D	);
			_("CADB", C, A, D, B	);
			_("CBAD", C, B, A, D	);
			_("CBDA", C, B, D, A	);
			_("CDAB", C, D, A, B	);
			_("CDBA", C, D, B, A	);
			_("DABC", D, A, B, C	);
			_("DACB", D, A, C, B	);
			_("DBAC", D, B, A, C	);
			_("DBCA", D, B, C, A	);
			_("DCAB", D, C, A, B	);
			_("DCBA", D, C, B, A	);
		}
	};



	template<>
	struct Permutation<5>{
		constexpr static size_t N = 5 + 1;

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
					std::string_view a = "", std::string_view b = "", std::string_view c = "", std::string_view d = "", std::string_view e = ""){

			if (a.empty() || txt[0] == '_')
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

		template<typename Func>
		static void for_each(std::string_view separator, std::string_view keyN, std::string_view keySub, std::array<std::string_view, N> const &indexes, Func func){
			auto const [A, B, C, D, E, S] = indexes;

			auto _ = [&](std::string_view txt,
						std::string_view a = "",
						std::string_view b = "",
						std::string_view c = "",
						std::string_view d = "",
						std::string_view e = ""){

				auto const [A, B, C, D, E, S] = indexes;

				hm4::PairBufferKey bufferKey;

				auto const key = concatenateBuffer(bufferKey,
						keyN	,		separator	,
						txt	,		separator	,
							a	,	separator	,
							b	,	separator	,
							c	,	separator	,
							d	,	separator	,
							e	,	separator	,
							S	,	separator	,
						keySub
				);

				func(key);
			};

			// 0 =>    1
			// 1 =>    5
			// 2 =>   20
			// 3 =>   60
			// 4 =>  120
			// 5 =>  120
			// Total 326 permutations total

			_("_"			);

			_("A",     A		);
			_("B",     B		);
			_("C",     C		);
			_("D",     D		);
			_("E",     E		);

			_("AB",    A, B		);
			_("BA",    B, A		);
			_("AC",    A, C		);
			_("CA",    C, A		);
			_("AD",    A, D		);
			_("DA",    D, A		);
			_("AE",    A, E		);
			_("EA",    E, A		);
			_("BC",    B, C		);
			_("CB",    C, B		);
			_("BD",    B, D		);
			_("DB",    D, B		);
			_("BE",    B, E		);
			_("EB",    E, B		);
			_("CD",    C, D		);
			_("DC",    D, C		);
			_("CE",    C, E		);
			_("EC",    E, C		);
			_("DE",    D, E		);
			_("ED",    E, D		);

			_("ABC",   A, B, C	);
			_("ACB",   A, C, B	);
			_("BAC",   B, A, C	);
			_("BCA",   B, C, A	);
			_("CAB",   C, A, B	);
			_("CBA",   C, B, A	);
			_("ABD",   A, B, D	);
			_("ADB",   A, D, B	);
			_("BAD",   B, A, D	);
			_("BDA",   B, D, A	);
			_("DAB",   D, A, B	);
			_("DBA",   D, B, A	);
			_("ABE",   A, B, E	);
			_("AEB",   A, E, B	);
			_("BAE",   B, A, E	);
			_("BEA",   B, E, A	);
			_("EAB",   E, A, B	);
			_("EBA",   E, B, A	);
			_("ACD",   A, C, D	);
			_("ADC",   A, D, C	);
			_("CAD",   C, A, D	);
			_("CDA",   C, D, A	);
			_("DAC",   D, A, C	);
			_("DCA",   D, C, A	);
			_("ACE",   A, C, E	);
			_("AEC",   A, E, C	);
			_("CAE",   C, A, E	);
			_("CEA",   C, E, A	);
			_("EAC",   E, A, C	);
			_("ECA",   E, C, A	);
			_("ADE",   A, D, E	);
			_("AED",   A, E, D	);
			_("DAE",   D, A, E	);
			_("DEA",   D, E, A	);
			_("EAD",   E, A, D	);
			_("EDA",   E, D, A	);
			_("BCD",   B, C, D	);
			_("BDC",   B, D, C	);
			_("CBD",   C, B, D	);
			_("CDB",   C, D, B	);
			_("DBC",   D, B, C	);
			_("DCB",   D, C, B	);
			_("BCE",   B, C, E	);
			_("BEC",   B, E, C	);
			_("CBE",   C, B, E	);
			_("CEB",   C, E, B	);
			_("EBC",   E, B, C	);
			_("ECB",   E, C, B	);
			_("BDE",   B, D, E	);
			_("BED",   B, E, D	);
			_("DBE",   D, B, E	);
			_("DEB",   D, E, B	);
			_("EBD",   E, B, D	);
			_("EDB",   E, D, B	);
			_("CDE",   C, D, E	);
			_("CED",   C, E, D	);
			_("DCE",   D, C, E	);
			_("DEC",   D, E, C	);
			_("ECD",   E, C, D	);
			_("EDC",   E, D, C	);

			_("ABCD",  A, B, C, D	);
			_("ABDC",  A, B, D, C	);
			_("ACBD",  A, C, B, D	);
			_("ACDB",  A, C, D, B	);
			_("ADBC",  A, D, B, C	);
			_("ADCB",  A, D, C, B	);
			_("BACD",  B, A, C, D	);
			_("BADC",  B, A, D, C	);
			_("BCAD",  B, C, A, D	);
			_("BCDA",  B, C, D, A	);
			_("BDAC",  B, D, A, C	);
			_("BDCA",  B, D, C, A	);
			_("CABD",  C, A, B, D	);
			_("CADB",  C, A, D, B	);
			_("CBAD",  C, B, A, D	);
			_("CBDA",  C, B, D, A	);
			_("CDAB",  C, D, A, B	);
			_("CDBA",  C, D, B, A	);
			_("DABC",  D, A, B, C	);
			_("DACB",  D, A, C, B	);
			_("DBAC",  D, B, A, C	);
			_("DBCA",  D, B, C, A	);
			_("DCAB",  D, C, A, B	);
			_("DCBA",  D, C, B, A	);
			_("ABCE",  A, B, C, E	);
			_("ABEC",  A, B, E, C	);
			_("ACBE",  A, C, B, E	);
			_("ACEB",  A, C, E, B	);
			_("AEBC",  A, E, B, C	);
			_("AECB",  A, E, C, B	);
			_("BACE",  B, A, C, E	);
			_("BAEC",  B, A, E, C	);
			_("BCAE",  B, C, A, E	);
			_("BCEA",  B, C, E, A	);
			_("BEAC",  B, E, A, C	);
			_("BECA",  B, E, C, A	);
			_("CABE",  C, A, B, E	);
			_("CAEB",  C, A, E, B	);
			_("CBAE",  C, B, A, E	);
			_("CBEA",  C, B, E, A	);
			_("CEAB",  C, E, A, B	);
			_("CEBA",  C, E, B, A	);
			_("EABC",  E, A, B, C	);
			_("EACB",  E, A, C, B	);
			_("EBAC",  E, B, A, C	);
			_("EBCA",  E, B, C, A	);
			_("ECAB",  E, C, A, B	);
			_("ECBA",  E, C, B, A	);
			_("ABDE",  A, B, D, E	);
			_("ABED",  A, B, E, D	);
			_("ADBE",  A, D, B, E	);
			_("ADEB",  A, D, E, B	);
			_("AEBD",  A, E, B, D	);
			_("AEDB",  A, E, D, B	);
			_("BADE",  B, A, D, E	);
			_("BAED",  B, A, E, D	);
			_("BDAE",  B, D, A, E	);
			_("BDEA",  B, D, E, A	);
			_("BEAD",  B, E, A, D	);
			_("BEDA",  B, E, D, A	);
			_("DABE",  D, A, B, E	);
			_("DAEB",  D, A, E, B	);
			_("DBAE",  D, B, A, E	);
			_("DBEA",  D, B, E, A	);
			_("DEAB",  D, E, A, B	);
			_("DEBA",  D, E, B, A	);
			_("EABD",  E, A, B, D	);
			_("EADB",  E, A, D, B	);
			_("EBAD",  E, B, A, D	);
			_("EBDA",  E, B, D, A	);
			_("EDAB",  E, D, A, B	);
			_("EDBA",  E, D, B, A	);
			_("ACDE",  A, C, D, E	);
			_("ACED",  A, C, E, D	);
			_("ADCE",  A, D, C, E	);
			_("ADEC",  A, D, E, C	);
			_("AECD",  A, E, C, D	);
			_("AEDC",  A, E, D, C	);
			_("CADE",  C, A, D, E	);
			_("CAED",  C, A, E, D	);
			_("CDAE",  C, D, A, E	);
			_("CDEA",  C, D, E, A	);
			_("CEAD",  C, E, A, D	);
			_("CEDA",  C, E, D, A	);
			_("DACE",  D, A, C, E	);
			_("DAEC",  D, A, E, C	);
			_("DCAE",  D, C, A, E	);
			_("DCEA",  D, C, E, A	);
			_("DEAC",  D, E, A, C	);
			_("DECA",  D, E, C, A	);
			_("EACD",  E, A, C, D	);
			_("EADC",  E, A, D, C	);
			_("ECAD",  E, C, A, D	);
			_("ECDA",  E, C, D, A	);
			_("EDAC",  E, D, A, C	);
			_("EDCA",  E, D, C, A	);
			_("BCDE",  B, C, D, E	);
			_("BCED",  B, C, E, D	);
			_("BDCE",  B, D, C, E	);
			_("BDEC",  B, D, E, C	);
			_("BECD",  B, E, C, D	);
			_("BEDC",  B, E, D, C	);
			_("CBDE",  C, B, D, E	);
			_("CBED",  C, B, E, D	);
			_("CDBE",  C, D, B, E	);
			_("CDEB",  C, D, E, B	);
			_("CEBD",  C, E, B, D	);
			_("CEDB",  C, E, D, B	);
			_("DBCE",  D, B, C, E	);
			_("DBEC",  D, B, E, C	);
			_("DCBE",  D, C, B, E	);
			_("DCEB",  D, C, E, B	);
			_("DEBC",  D, E, B, C	);
			_("DECB",  D, E, C, B	);
			_("EBCD",  E, B, C, D	);
			_("EBDC",  E, B, D, C	);
			_("ECBD",  E, C, B, D	);
			_("ECDB",  E, C, D, B	);
			_("EDBC",  E, D, B, C	);
			_("EDCB",  E, D, C, B	);

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

				if (!shared::index_token::valid(indexesOld)){
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

			if (!shared::index_token::valid(indexesOld)){
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

		static_assert(std::is_same_v<Permutation, Permutation1NoIndex>);

		hm4::PairBufferKey bufferKeyCtrl;
		auto const keyCtrl = makeKeyCtrl(bufferKeyCtrl, DBAdapter::SEPARATOR, keyN, keySub);

		logger<Logger::DEBUG>() << "ZSetMulti::GET: ctrl key" << keyCtrl;

		if (auto const encodedValue = hm4::getPairVal(*db, keyCtrl); !encodedValue.empty()){
			// Case 1: ctrl key is set

			if constexpr(std::is_same_v<IndexController, std::nullptr_t>){
				using namespace impl_;

				auto const indexes = decodeIndex<Permutation, IndexController>(DBAdapter::SEPARATOR, encodedValue);

				if (shared::index_token::valid(keyN, keySub) && shared::index_token::valid(indexes)){
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

			if (shared::index_token::valid(keyN, keySub) && shared::index_token::valid(indexes))
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

		if (!shared::index_token::valid(keyN))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		auto const varg = 2;

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
			if (auto const &keySub = *itk; !shared::index_token::valid(keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

		for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			auto const &keySub = *itk;

			rem<Permutation, IndexController>(db, keyN, keySub);
		}

		return result.set_1();
	}

} // net::worker::shared::zset

#endif

