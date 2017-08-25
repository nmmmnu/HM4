#ifndef MY_INI_FILE_
#define MY_INI_FILE_

#include <fstream>

#include "stringref.h"

template<class PROCESSOR>
bool readINIFile(std::istream &file, PROCESSOR &processor);

template<class PROCESSOR>
bool readINIFile(const StringRef &filename, PROCESSOR &processor);


// ===================================


template<class PROCESSOR>
bool readINIFile(const StringRef &filename){
	PROCESSOR processor;
	return readINIFile(filename, processor);
}


template<class PROCESSOR>
bool readINIFile(std::istream &file){
	PROCESSOR processor;
	return readINIFile(file, processor);
}


// ===================================

#include "inifile_impl.h"

#endif

