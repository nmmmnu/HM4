#include "idgenerator.h"

#include <iostream>
#include <iomanip>

template <class T>
void test(T id, const char *name, unsigned count = 10){
	char buffer[T::BUFFER_SIZE];

	std::cout	<< name
			<< '\n';

	for(unsigned i = 0; i < count; ++i)
		std::cout	<< std::setw (3) << i
				<< " => "
				<< id(buffer)
				<< '\n';
}

int main(){
	namespace idg = hm4::idgenerator;

	test( idg::IDGeneratorTS_HEX{},	"IDGeneratorTS@Hex"	);
	test( idg::IDGeneratorTS_DEC{},	"IDGeneratorTS@Dec"	);
	test( idg::IDGeneratorDate{},	"IDGeneratorDate"	);
}

