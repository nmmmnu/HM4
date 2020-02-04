#include <string_view>

std::string_view base64_decode(const void *data, size_t const len, char *buffer){
	constexpr unsigned char base60_table[256] = {
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0, 62, 63, 62, 62, 63,
		52, 53, 54, 55, 56, 57, 58, 59,
		60, 61,  0,  0,  0,  0,  0,  0,
		 0,  0,  1,  2,  3,  4,  5,  6,
		 7,  8,  9, 10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22,
		23, 24, 25,  0,  0,  0,  0, 63,
		 0, 26, 27, 28, 29, 30, 31, 32,
		33, 34, 35, 36, 37, 38, 39, 40,
		41, 42, 43, 44, 45, 46, 47, 48,
		49, 50, 51
	};

	unsigned char *p = (unsigned char *) data;

	int const pad = len > 0 && (len % 4 || p[len - 1] == '=');

	size_t const L = ((len + 3) / 4 - pad) * 4;

	size_t buffer_size = 0;

	for (size_t i = 0; i < L; i += 4){
		int const n =	base60_table[p[i + 0]] << 18 |
				base60_table[p[i + 1]] << 12 |
				base60_table[p[i + 2]] <<  6 |
				base60_table[p[i + 3]] <<  0
		;

		buffer[buffer_size++] = n >> 16 & 0xFF;
		buffer[buffer_size++] = n >>  8 & 0xFF;
		buffer[buffer_size++] = n >>  0 & 0xFF;
	}

	if (pad){
		int n =		base60_table[p[L + 0]] << 18 |
				base60_table[p[L + 1]] << 12
		;

		buffer[buffer_size++] = n >> 16 & 0xFF;

		if (len > L + 2 && p[L + 2] != '='){
		n |=		base60_table[p[L + 2]] <<  6
		;

		buffer[buffer_size++] = n >>  8 & 0xFF;
		}
	}

	return { buffer, buffer_size };
}

#include <iostream>

int main(){
	constexpr std::string_view s =
	//	"aGVsbG8gdGhpcyBpcyBvcmlnaW5hbCBzdHJpbmchISEgOi0p"
	//	"aGVsbG8gdGhpcyBpcyBvcmlnaW5hbCBzdHJpbmchIDotKQ=="
		"aGVsbG8gdGhpcyBpcyBvcmlnaW5hbCBzdHJpbmchISA6LSk="
	;

	std::string buffer( (size_t) 100, '*' );

	std::cout << ">>" << base64_decode(s.data(), s.size(), buffer.data()) << "<<" << '\n';
}


