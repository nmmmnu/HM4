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


class INIFileAssignHelper{
public:
	INIFileAssignHelper(const StringRef &name, const StringRef &value) :
					name(name), value(value){}

	template<typename T>
	bool setOption(const char *opt_name, T &param) const{
		if (name == opt_name){
			assign_(param);
			return true;
		}

		return false;
	}

private:
	template<class T>
	void assign_(T &param) const{
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");

		auto const default_value = param;

		param = stou<T>(value, default_value);
	}

	void assign_(std::string &param) const{
		param = value;
	}

	void assign_(nullptr_t) const{
		/* nada */
	}

private:
	const StringRef &name;
	const StringRef &value;
};


// ===================================


#include "inifile_impl.h"


#endif

