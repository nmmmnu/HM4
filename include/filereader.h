#ifndef FILE_READER_H_
#define FILE_READER_H_

#include <fstream>

#include "stringref.h"

class BasicFileReader{
public:
	using Options = unsigned char;

public:
	constexpr static Options OPTION_TRIM		= 1 << 0;
	constexpr static Options OPTION_SKIP_EMPRY	= 1 << 1;

	constexpr static Options DEFAULT_OPTIONS	= OPTION_TRIM | OPTION_SKIP_EMPRY;

protected:
	BasicFileReader() = default;

protected:
	static std::string &trim__(std::string &line){
		constexpr const char *trim_ch = " \t\r\n";

		line.erase(line.find_last_not_of(trim_ch) + 1);

		return line;
	}
};



class FileReader : public BasicFileReader{
private:
	constexpr static const char *NAME = "File Reader using standard streams";

public:
	FileReader(const StringRef &filename, Options const options = DEFAULT_OPTIONS) :
					file_(filename),
					options_(options){}

	std::string getLine() {
		std::string line;

		while( getline(file_, line) ){
			if (options_ & OPTION_TRIM)
				trim__(line);

			if ((options_ & OPTION_SKIP_EMPRY) && line.empty() )
				continue;

			return line;
		}

		// return empty line...
		return line;
	}

	operator bool() const{
		return file_.good();
	}

public:
	static const char *name(){
		return NAME;
	}

private:
	std::ifstream	file_;
	Options		options_;
};

#endif

