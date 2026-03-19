#include "idgenerator.h"

#include <iostream>
#include <iomanip>

template <class T>
void test(T id, const char *name, unsigned count = 10){
	typename T::to_string_buffer_t buffer;

	std::cout	<< name
			<< '\n';

	for(unsigned i = 0; i < count; ++i)
		std::cout	<< std::setw (3) << i
				<< " => "
				<< id(buffer)
				<< '\n';
}

int main(){
	namespace idg = idgenerator;

	uint8_t const serverID = 0xA5;

	test( idg::IDGeneratorTS_HEX		{ serverID },	"IDGeneratorTS@Hex"	);
	test( idg::IDGeneratorTS_HEXMono	{ serverID },	"IDGeneratorTS@HexMono"	);
	test( idg::IDGeneratorTS_DEC		{ serverID },	"IDGeneratorTS@Dec"	);
	test( idg::IDGeneratorDate		{ serverID },	"IDGeneratorDate"	);
	test( idg::IDGeneratorDateMono		{ serverID },	"IDGeneratorDateMono"	);
}

