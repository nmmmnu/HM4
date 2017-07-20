#include <iostream>
#include <fstream>
#include <cstring>

int main(){
	std::ifstream file("/DATA/home/nmmm/Development/HM4/a");

	const size_t max = 100;

	char s[100];

	file.getline(s, max );

	std::cout << file.gcount() << '\n';

	std::cout << strlen(s) << '\n';
}

