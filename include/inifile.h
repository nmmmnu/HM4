#ifndef MY_INI_FILE_
#define MY_INI_FILE_

#include <fstream>

#include <string_view>

template<class PROCESSOR>
bool readINIFile(std::istream &file, PROCESSOR &processor);

template<class PROCESSOR>
bool readINIFile(std::string_view const filename, PROCESSOR &processor);


// ===================================


template<class PROCESSOR>
bool readINIFile(std::string_view const filename){
	PROCESSOR processor;
	return readINIFile(filename, processor);
}


template<class PROCESSOR>
bool readINIFile(std::istream &file){
	PROCESSOR processor;
	return readINIFile(file, processor);
}


// ===================================


#include "inifile.h.cc"


#endif

