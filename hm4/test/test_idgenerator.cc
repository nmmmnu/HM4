#include "idgenerator/idgeneratorts.h"
#include "idgenerator/idgeneratordate.h"

#include <iostream>
#include <iomanip>

template <class T>
void test(T id, const char *name, unsigned count = 10){
	std::cout	<< name
			<< '\n';

	for(unsigned i = 0; i < count; ++i)
		std::cout	<< std::setw (3) << i
				<< " => "
				<< id()
				<< '\n';
}

int main(){
	namespace idg = hm4::idgenerator;

	test( idg::IDGeneratorTS{ true },	"IDGeneratorTS@Hex"	);
	test( idg::IDGeneratorTS{ false },	"IDGeneratorTS@Dec"	);
	test( idg::IDGeneratorDate{},		"IDGeneratorDate"	);
}

