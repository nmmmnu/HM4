#include "mytest.h"
#include "iobuffer.h"

//#include "simplememorymanager.h"

#include "tieredmemorymanager.h"
#include "allocatedbuffer.h"

MyTest mytest;

//using IOBuffer = net::IOBuffer<memory_manager::SimpleMemoryManager>;
using IOBuffer = net::IOBuffer<memory_manager::TieredMemoryManager<MyBuffer::AllocatedMemoryResourceOwned<> > >;

namespace {
	void testPush(IOBuffer &io, typename IOBuffer::MemoryManager &mm, std::string_view sv, std::string &bs, bool expect = true){
		bool const result = io.push(mm, sv);

		if (expect)
			bs += sv;

		mytest("push",	result == expect			);
		mytest("chk",	bs == static_cast<std::string_view>(io)	);

		io.print();
	}


	void testPop(IOBuffer &io, typename IOBuffer::MemoryManager &mm, size_t const size, std::string &bs, bool expect = true){
		bool const result = io.pop(mm, size);

		if (expect)
			bs.erase(0, size);

		mytest("pop", result == expect				);
		mytest("chk",	bs == static_cast<std::string_view>(io)	);

		io.print();
	}

} // anonymous namespace

size_t const MB = 1024 * 1024;

int main(){
	mytest.begin("iobuffer");

	std::string b;

	typename IOBuffer::MemoryManager mm;

	typename IOBuffer::MemoryManager::Node *node = 0;

	mm.allocateChain(node, 4 * MB);

	mm.deallocate(node);

	return 0;

	IOBuffer io;

	testPush(io, mm, "aaa"	, b 		);
	testPush(io, mm, ""	, b , false	);
	testPush(io, mm, "bbb"	, b 		);
	testPush(io, mm, "ccc"	, b 		);

	testPop (io, mm, 3	, b 		);
	testPop (io, mm, 0	, b , false	);
	testPop (io, mm, 3	, b 		);
	testPop (io, mm, 3	, b 		);

	io.deallocate(mm);

	mytest.end();
}

