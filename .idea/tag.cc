#include <type_traits>
#include <iostream>

struct A{
	template<typename T>
	void process(T a){
		process_(a, std::true_type{});
	}

	template<typename T>
	void process_(T a, std::true_type){
		std::cout << "true" << '\n';
	}

	template<typename T>
	void process_(T a, std::false_type){
		std::cout << "false" << '\n';
	}
};


int main(){
	A a;

	a.process(5);
}

