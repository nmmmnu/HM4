#include "base64.h"

#include <array>

// https://nachtimwald.com/2017/11/18/base64-encode-and-decode-in-c/

namespace{
	constexpr std::string_view b64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	constexpr auto generateDecodeTable__(){
		std::array<int, 80> inv{};

		for (int &x : inv)
			x = -1;

		for (size_t i = 0; i < b64chars.size(); ++i)
			inv[ size_t(b64chars[i] - 43) ] = (int) i;

		return inv;
	}

	constexpr std::array<int, 80> b64invs = generateDecodeTable__();

	constexpr bool isValidChar__(char const c){
		if (c >= '0' && c <= '9')
			return true;

		if (c >= 'A' && c <= 'Z')
			return true;

		if (c >= 'a' && c <= 'z')
			return true;

		if (c == '+' || c == '/' || c == '=')
			return true;

		return false;
	}

	#if 0
	constexpr int b64invs[] = {
		62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57, 58,
		59, 60, 61, -1, -1, -1, -1, -1, -1, -1,  0,  1,
		 2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
		14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
		-1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51
	};
	#endif

	[[maybe_unused]]
	constexpr size_t encoded_size(size_t const size){
		size_t ret = size;

		if (size % 3 != 0)
			ret += 3 - (size % 3);

		ret /= 3;
		ret *= 4;

		return ret;
	}

} // namespace



std::string_view base64_decode(const char *data, size_t size, char *buffer){
	// const char *in, unsigned char *out

	if (size == 0)
		return {};

	if (size % 4 != 0)
		return {};

	for (size_t i = 0; i < size; ++i)
		if (!isValidChar__(data[i]))
			return {};

	const unsigned char *input = (const unsigned char *) data;

	size_t len = 0;

	for (size_t i = 0, j = 0; i < size; i += 4, j += 3) {
		int v = 0;
		v = (v << 6) | b64invs[ input[i + 0] - 43 ];
		v = (v << 6) | b64invs[ input[i + 1] - 43 ];

		if (input[i + 2] == '=')
		v = (v << 6);
		else
		v = (v << 6) | b64invs[ input[i + 2] - 43 ];

		if (input[i + 3] == '=')
		v = (v << 6);
		else
		v = (v << 6) | b64invs[ input[i + 3] - 43 ];

		buffer[j + 0] = char( (v >> 16) & 0xFF );
		++len;

		if (input[i + 2] != '='){
		buffer[j + 1] = char( (v >>  8) & 0xFF );
		++len;
		}

		if (input[i + 3] != '='){
		buffer[j + 2] = char( (v >>  0) & 0xFF );
		++len;
		}
	}

	return { buffer, len };
}


