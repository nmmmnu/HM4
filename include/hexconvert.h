#ifndef HEX_CONVERT_H_
#define HEX_CONVERT_H_

#include <cstdint>
#include <type_traits>
#include <array>
#include <string_view>

#include "my_type_traits.h"

#include "uint128_t.h"

namespace hex_convert{
	namespace options{
		using type = uint8_t;

		constexpr type nonterminate	= 0b0000;
		constexpr type terminate	= 0b0001;

		constexpr type lowercase	= 0b0000;
		constexpr type uppercase	= 0b0010;

		constexpr type defaults	= terminate | uppercase;
	}

	template <typename T, options::type opt = options::defaults>
	constexpr std::string_view toHex(T const number, char *buffer){
		static_assert( is_any_v<T, uint8_t, uint16_t, uint32_t, uint64_t, uint128_t> );

		constexpr const char *digits = [](){
			if constexpr(opt & options::uppercase)
				return "0123456789ABCDEF";
			else
				return "0123456789abcdef";
		}();

		constexpr auto size = sizeof(T) * 2;

		if constexpr(opt & options::terminate)
			buffer[size] = '\0';

		for (std::size_t i = 0; i < size; ++i){
			auto const index = (number >> (4 * i)) & 0xF;
			buffer[size - 1 - i] = digits[index];
		}

		return { buffer, size };
	}

	template <typename T, options::type opt = options::defaults, size_t N>
	constexpr std::string_view toHex(T const number, std::array<char, N> &buffer){
		static_assert( is_any_v<T, uint8_t, uint16_t, uint32_t, uint64_t, uint128_t> );

		static_assert(N >= sizeof(T) * 2 + (opt & options::terminate ? 1 : 0));

		return toHex<T, opt>(number, buffer.data());
	}

	template <typename T>
	constexpr auto fromHex(std::string_view const hex){
		static_assert( is_any_v<T, uint8_t, uint16_t, uint32_t, uint64_t, uint128_t> );

		auto _ = [](char c) -> T{
			switch(c){
			default:
			case '0': return 0;
			case '1': return 1;
			case '2': return 2;
			case '3': return 3;
			case '4': return 4;
			case '5': return 5;
			case '6': return 6;
			case '7': return 7;
			case '8': return 8;
			case '9': return 9;
			case 'A': case 'a': return 10;
			case 'B': case 'b': return 11;
			case 'C': case 'c': return 12;
			case 'D': case 'd': return 13;
			case 'E': case 'e': return 14;
			case 'F': case 'f': return 15;
			}
		};

		T val = 0;

		for(auto const &c : hex){
			T const nibble = _(c);

			// cast added for silence the clang warning
			val = T(val << 4) | nibble;
		}

		return val;
	}

	namespace test{
		template<typename T, uint8_t opt>
		constexpr bool toHexTest(T number, std::string_view correct){
			char buffer[32]{};

			return toHex<T, opt | options::terminate>(number, buffer) == correct;
		}

		constexpr auto up = options::uppercase;
		constexpr auto lo = options::lowercase;

		static_assert(toHexTest<uint8_t , up>(0x00			, "00"			));
		static_assert(toHexTest<uint8_t , up>(0x05			, "05"			));
		static_assert(toHexTest<uint8_t , up>(0xFF			, "FF"			));
		static_assert(toHexTest<uint8_t , lo>(0xFF			, "ff"			));

		static_assert(toHexTest<uint16_t, up>(0x00			, "0000"		));
		static_assert(toHexTest<uint16_t, up>(0x05			, "0005"		));
		static_assert(toHexTest<uint16_t, up>(0xABBA			, "ABBA"		));
		static_assert(toHexTest<uint16_t, lo>(0xABBA			, "abba"		));
		static_assert(toHexTest<uint16_t, up>(0xFFFF			, "FFFF"		));
		static_assert(toHexTest<uint16_t, lo>(0xFFFF			, "ffff"		));

		static_assert(toHexTest<uint32_t, up>(0x00			, "00000000"		));
		static_assert(toHexTest<uint32_t, up>(0x05			, "00000005"		));
		static_assert(toHexTest<uint32_t, up>(0xABBA			, "0000ABBA"		));
		static_assert(toHexTest<uint32_t, lo>(0xABBA			, "0000abba"		));
		static_assert(toHexTest<uint32_t, up>(0xDEAD'BEEF		, "DEADBEEF"		));
		static_assert(toHexTest<uint32_t, lo>(0xDEAD'BEEF		, "deadbeef"		));
		static_assert(toHexTest<uint32_t, up>(0xFFFF'FFFF		, "FFFFFFFF"		));
		static_assert(toHexTest<uint32_t, lo>(0xFFFF'FFFF		, "ffffffff"		));

		static_assert(toHexTest<uint64_t, up>(0x00			, "0000000000000000"	));
		static_assert(toHexTest<uint64_t, up>(0x05			, "0000000000000005"	));
		static_assert(toHexTest<uint64_t, up>(0xABBA			, "000000000000ABBA"	));
		static_assert(toHexTest<uint64_t, lo>(0xABBA			, "000000000000abba"	));
		static_assert(toHexTest<uint64_t, up>(0xDEAD'BEEF		, "00000000DEADBEEF"	));
		static_assert(toHexTest<uint64_t, lo>(0xDEAD'BEEF		, "00000000deadbeef"	));
		static_assert(toHexTest<uint64_t, up>(0xFFFF'FFFF'FFFF'FFFF	, "FFFFFFFFFFFFFFFF"	));
		static_assert(toHexTest<uint64_t, lo>(0xFFFF'FFFF'FFFF'FFFF	, "ffffffffffffffff"	));



		template<typename T>
		struct Buffer;

		template<> struct Buffer<uint8_t >{ char buffer[32] = ">XX<"			; };
		template<> struct Buffer<uint16_t>{ char buffer[32] = ">XXXX<"			; };
		template<> struct Buffer<uint32_t>{ char buffer[32] = ">XXXXXXXX<"		; };
		template<> struct Buffer<uint64_t>{ char buffer[32] = ">XXXXXXXXXXXXXXXX<"	; };

		template<typename T, uint8_t opt>
		constexpr bool toHexTestInside(T number, std::string_view correct){
			Buffer<T> b;
			toHex<T, opt | options::nonterminate>(number, b.buffer + 1);
			return b.buffer == correct;
		}

		static_assert(toHexTestInside<uint8_t , up>(0x00			, ">"	"00"			"<" ));
		static_assert(toHexTestInside<uint8_t , up>(0x05			, ">"	"05"			"<" ));
		static_assert(toHexTestInside<uint8_t , up>(0xFF			, ">"	"FF"			"<" ));
		static_assert(toHexTestInside<uint8_t , lo>(0xFF			, ">"	"ff"			"<" ));

		static_assert(toHexTestInside<uint16_t, up>(0x00			, ">"	"0000"			"<" ));
		static_assert(toHexTestInside<uint16_t, up>(0x05			, ">"	"0005"			"<" ));
		static_assert(toHexTestInside<uint16_t, up>(0xABBA			, ">"	"ABBA"			"<" ));
		static_assert(toHexTestInside<uint16_t, lo>(0xABBA			, ">"	"abba"			"<" ));
		static_assert(toHexTestInside<uint16_t, up>(0xFFFF			, ">"	"FFFF"			"<" ));
		static_assert(toHexTestInside<uint16_t, lo>(0xFFFF			, ">"	"ffff"			"<" ));

		static_assert(toHexTestInside<uint32_t, up>(0x00			, ">"	"00000000"		"<" ));
		static_assert(toHexTestInside<uint32_t, up>(0x05			, ">"	"00000005"		"<" ));
		static_assert(toHexTestInside<uint32_t, up>(0xABBA			, ">"	"0000ABBA"		"<" ));
		static_assert(toHexTestInside<uint32_t, lo>(0xABBA			, ">"	"0000abba"		"<" ));
		static_assert(toHexTestInside<uint32_t, up>(0xDEAD'BEEF			, ">"	"DEADBEEF"		"<" ));
		static_assert(toHexTestInside<uint32_t, lo>(0xDEAD'BEEF			, ">"	"deadbeef"		"<" ));
		static_assert(toHexTestInside<uint32_t, up>(0xFFFF'FFFF			, ">"	"FFFFFFFF"		"<" ));
		static_assert(toHexTestInside<uint32_t, lo>(0xFFFF'FFFF			, ">"	"ffffffff"		"<" ));

		static_assert(toHexTestInside<uint64_t, up>(0x00			, ">"	"0000000000000000"	"<" ));
		static_assert(toHexTestInside<uint64_t, up>(0x05			, ">"	"0000000000000005"	"<" ));
		static_assert(toHexTestInside<uint64_t, up>(0xABBA			, ">"	"000000000000ABBA"	"<" ));
		static_assert(toHexTestInside<uint64_t, lo>(0xABBA			, ">"	"000000000000abba"	"<" ));
		static_assert(toHexTestInside<uint64_t, up>(0xDEAD'BEEF			, ">"	"00000000DEADBEEF"	"<" ));
		static_assert(toHexTestInside<uint64_t, lo>(0xDEAD'BEEF			, ">"	"00000000deadbeef"	"<" ));
		static_assert(toHexTestInside<uint64_t, up>(0xFFFF'FFFF'FFFF'FFFF	, ">"	"FFFFFFFFFFFFFFFF"	"<" ));
		static_assert(toHexTestInside<uint64_t, lo>(0xFFFF'FFFF'FFFF'FFFF	, ">"	"ffffffffffffffff"	"<" ));



		static_assert(fromHex<uint8_t >("0"			) == 0x00			);
		static_assert(fromHex<uint8_t >("00"			) == 0x00			);
		static_assert(fromHex<uint8_t >("5"			) == 0x05			);
		static_assert(fromHex<uint8_t >("FF"			) == 0xFF			);

		static_assert(fromHex<uint16_t>("0"			) == 0x00			);
		static_assert(fromHex<uint16_t>("00"			) == 0x00			);
		static_assert(fromHex<uint16_t>("5"			) == 0x05			);
		static_assert(fromHex<uint16_t>("abba"			) == 0xABBA			);
		static_assert(fromHex<uint16_t>("ABBA"			) == 0xABBA			);
		static_assert(fromHex<uint16_t>("FFFF"			) == 0xFFFF			);

		static_assert(fromHex<uint32_t>("0"			) == 0x00			);
		static_assert(fromHex<uint32_t>("00"			) == 0x00			);
		static_assert(fromHex<uint32_t>("5"			) == 0x05			);
		static_assert(fromHex<uint32_t>("abba"			) == 0xABBA			);
		static_assert(fromHex<uint32_t>("ABBA"			) == 0xABBA			);
		static_assert(fromHex<uint32_t>("deadbeef"		) == 0xDEAD'BEEF		);
		static_assert(fromHex<uint32_t>("DEADBEEF"		) == 0xDEAD'BEEF		);
		static_assert(fromHex<uint32_t>("FFFFFFFF"		) == 0xFFFF'FFFF		);

		static_assert(fromHex<uint64_t>("0"			) == 0x00			);
		static_assert(fromHex<uint64_t>("00"			) == 0x00			);
		static_assert(fromHex<uint64_t>("5"			) == 0x05			);
		static_assert(fromHex<uint64_t>("abba"			) == 0xABBA			);
		static_assert(fromHex<uint64_t>("ABBA"			) == 0xABBA			);
		static_assert(fromHex<uint64_t>("deadbeef"		) == 0xDEAD'BEEF		);
		static_assert(fromHex<uint64_t>("DEADBEEF"		) == 0xDEAD'BEEF		);
		static_assert(fromHex<uint64_t>("FFFFFFFFFFFFFFFF"	) == 0xFFFF'FFFF'FFFF'FFFF	);
	}
}

#endif

