#include "base64.h"

#include <cstdio>
#include <string>
#include <string_view>

namespace {
	void print_test(const char *name){
		printf("Shell base64 is broken, if you have PHP, here is quick test:\n");
		printf("(\n");

		constexpr std::string_view text = "/.../.../.../";

		for(size_t i = 0; i <= text.size(); ++i)
			printf("  %s \"$(php -r \"echo base64_encode('%s');\")\"\n", name, & text[i]);

		printf(")\n");
	}

	int print_usage(const char *name){
		printf("Usage:\n");
		printf("\t%s [text] - base64_decode string\n", name);

		print_test(name);

		return 1;
	}
} //namespace

#include <iostream>

int main(int argc, char **argv){
	if (argc < 2)
		return print_usage(argv[0]);

	std::string_view const data = argv[1];

	std::string buffer(data.size(), '*');

	std::cout << ">>>" << base64_decode(data, buffer.data()) << "<<<" << '\n';
}

