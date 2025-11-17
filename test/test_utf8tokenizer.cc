#include "utf8tokenizer.h"

#include <iostream>

template<typename TextTokenizer>
void test(std::string_view s){
	TextTokenizer v{ s };

	std::cout << '\n';

	std::cout << "↓↓↓↓↓" << '\n';

	for(auto it = std::begin(v); it != std::end(v); ++it)
		std::cout << '[' << *it << ']' << ' ' << '>' << v.to(it) << '<' << '\n';

	std::cout << "↑↑↑↑↑" << '\n';
}

int main() {
	test<UTF8Tokenizer	>("здравей καλημέρα Բարև გამარჯობა 你好 こんにちは 🅰🅱🅱🅰 ♠♣♥♦ grüß gott"	);
	test<UTF8Tokenizer	>("здравей καλημέρα Բարև გამარჯობა 你好 こんにちは 🅰🅱🅱🅰 ♠♣♥♦ grüß gott Ж"	);
	test<UTF8Tokenizer	>(""									);
	test<ASCIITokenizer	>("hello"								);
	test<ASCIITokenizer	>(""									);
}

