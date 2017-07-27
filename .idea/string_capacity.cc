#include <iostream>

int main(){
	std::string s;

	std::cout << s.capacity() << '\n';

	s.reserve(1000);

	std::cout << s.capacity() << '\n';

	s.reserve(2000);

	std::cout << s.capacity() << '\n';

	s.reserve(100);

	std::cout << s.capacity() << '\n';
}

