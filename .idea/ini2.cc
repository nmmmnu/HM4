#include <fstream>
#include <string>
#include <cassert>

template<class PROCESSOR>
class INIFile{
private:
	constexpr static const char *TRIM	= " \t\r\n";
	constexpr static const char *COMMENTS	= ";#";

	constexpr static char EQUAL		= '=';

public:
	INIFile(std::string filename, PROCESSOR &processor) :
					filename(std::move(filename)	),
					processor(processor		){}

	bool operator()(){
		assert(!filename.empty());

		std::ifstream file;
		file.open(filename);

		if (!file)
			return false;

		std::string line;

		while (std::getline(file, line)){
			removeComment__(line);
			parseLine_(line);
		}

		return true;
	}

private:
	bool parseLine_(const std::string &line){
		size_t const sep = line.find(EQUAL);

		if (sep == line.npos)
			return false;

		const std::string key = getString__(line, 0, sep);

		if (key.empty())
			return false;

		const std::string val = getString__(line, sep + 1, line.size());

		if (val.empty())
			return false;

		processor(key, val);

		return true;
	}

private:
	static void removeComment__(std::string &line){
		size_t const pos = line.find_first_of(COMMENTS);

		if (pos != line.npos)
			line.erase(pos);
	}

	static std::string getString__(const std::string &line, size_t const start, size_t const end){
		std::string s = line.substr(start, end);
		trim__(s);

		return s;
	}

private:
	static void trim__(std::string &s){
		s.erase(0, s.find_first_not_of(TRIM));
		s.erase(s.find_last_not_of(TRIM) + 1);
	}

private:
	std::string	filename;
	PROCESSOR	&processor;

};

template<class PROCESSOR>
bool readINIFile(std::string filename, PROCESSOR &processor){
	INIFile<PROCESSOR> ini(filename, processor);

	return ini();
}

#include <iostream>

struct PrintProcessor{
	void operator()(const std::string &key, const std::string &val) const{
		std::cout << '-' << key << '=' << val << '-' << '\n';
	}
};

int main(){
	PrintProcessor pp;
	readINIFile("ini.txt", pp);
}


