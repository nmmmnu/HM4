#include "ambuffer.h"
#include "tieredmemorymanager.h"
#include "allocatedbuffer.h"

#include <string_view>

std::string_view const sv = "Hello, how are you?";


int main(){
	using TMM = memory_manager::TieredMemoryManager<MyBuffer::AllocatedMemoryResourceOwned<> >;

	using TieredBuffer = am_buffer::AMBuffer<TMM>;

	TMM tmm{ 1024 };

	tmm.printShort();
	tmm.print();



	TieredBuffer b0{ tmm, 268500992 };	b0.print();
	TieredBuffer b1{ tmm, 268500992 };	b1.print();
	TieredBuffer b2{ tmm, 268500992 };	b2.print();

	TieredBuffer b3{ tmm,  16777216 };	b3.print();
	TieredBuffer b4{ tmm,  16777216 };	b4.print();



	TieredBuffer a{ tmm };			a.print();

	[[maybe_unused]]
	bool b;

	b = a.reserve(      111 + 0 );	a.print();

	b = a.push_(sv.data(), sv.size());

	b = a.reserve(      222 + 0 );	a.print();
	b = a.reserve(    16384 + 1 );	a.print();
	b = a.reserve(   262144 + 1 );	a.print();
	b = a.reserve( 16777216 + 0 );	a.print();
	b = a.reserve( 67108864 + 0 );	a.print();
	b = a.reserve( 67108864 + 1 );	a.print();

	printf(">>>%s<<<\n", a.data());
}

