#include "tieredmemorymanager.h"
#include "allocatedbuffer.h"
#include "iobuffer.h"

#include <string_view>

[[maybe_unused]]
std::string_view const sv = "Hello, how are you?";

int main(){
	using MM = memory_manager::TieredMemoryManager<MyBuffer::AllocatedMemoryResourceOwned<> >;

	using IOBuffer = net::IOBuffer<MM>;

	MM mm{ 1024 };

	mm.printShort();
	mm.print();

	IOBuffer a;	a.print();

	[[maybe_unused]]
	bool b;

	b = a.reserve(mm,      111 + 0 );	a.print();

//	b = a.push(sv.data(), sv.size());

	b = a.reserve(mm,      222 + 0 );	a.print();
	b = a.reserve(mm,    16384 + 1 );	a.print();
	b = a.reserve(mm,   262144 + 1 );	a.print();
	b = a.reserve(mm, 16777216 + 0 );	a.print();
	b = a.reserve(mm, 67108864 + 0 );	a.print();
	b = a.reserve(mm, 67108864 + 1 );	a.print();

	printf(">>>%s<<<\n", a.data());

	a.clear(mm);
}

