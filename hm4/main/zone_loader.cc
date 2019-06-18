#include <iostream>
#include <iomanip>

static int printUsage(const char *cmd){
	std::cout
		<< "Usage:"	<< '\n'
		<< "\t"		<< cmd	<< " [file.txt] - process file.txt"	<< '\n'

		<< '\n';

	return 10;
}

#include "filereader.h"
#include "stringtokenizer.h"


int main(int argc, char **argv){
	if (argc <= 1){
		return printUsage(argv[0]);
	}

	const char *filename	= argv[1];

	FileReader<1024> reader{ filename };

	while(reader){
		const std::string &line = reader.getLine();

		StringTokenizer const tok{ line, ':' };

		std::vector<StringRef> const parts{ std::begin(tok), std::end(tok) };

		if (parts.size() != 3 && parts.size() != 4){
			std::cerr << "Problem with line " << line << '\n';
			continue;
		}

		const StringRef &op	= { & parts[0][0], 1 };
		const StringRef &date	= parts[1];
		const StringRef &domain	= parts[2];

		// add date key

		std::cout
				<< "t" << ":" << date << ":" << domain
				<< '\t'
				<< 1
				<< '\n'
		;


		// add domain / ns key

		std::cout
				<< "d" << ":" << domain << ":" << date
				<< '\t'
		;


		if (parts.size() == 4){
			const auto &ns = parts[3];

			std::cout
				<< op
				<< ":"
				<< ns
				<< '\n'
			;

		}else{
			std::cout
				<< "d"
				<< '\n'
			;
		}
	}

	return 0;
}


