#include <fstream>
#include <utility>
#include <vector>
#include <cassert>

#include "stringref.h"
#include "stou.h"

#include <iostream>

class INIFile{
private:
	constexpr static char COMMENT	= ';';
	constexpr static char EQUAL	= '=';


	using INIPair = std::pair<std::string, std::string>;

public:
	INIFile() = default;

	INIFile(const StringRef &filename){
		readFile(filename);
	}

	void clear(){
		data_.clear();
	}

	bool readFile(const StringRef &filename){
		assert(!filename.empty());

		clear();

		std::ifstream file;
		file.open(filename);

		if (!file)
			return false;

		std::string line;

		while (std::getline(file, line)){
			if (line.empty())
				continue;

			parseLine_(line);
		}

		return true;
	}

	const std::string &get(const StringRef &key) const{
		assert(!key.empty());
		for(const auto &p : data_)
			if (p.first == key)
				return p.second;

		return null_;
	}

	template<typename T>
	T getInt(const StringRef &key, T const default_value = 0 ){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned");

		assert(!key.empty());

		const auto &val = get(key);

		if (val.empty())
			return default_value;

		return stou<T>(val.c_str());
	}
/*
	bool getBool(const StringRef &key, bool const default_value = false ){
		assert(!key.empty());
		return getInt<uint8_t>(key, default_value ? 1 : 0) != 0;
	}
*/
private:
	bool parseLine_(std::string &line){
		parseComment__(line);

		size_t const sep = line.find(EQUAL);

		if (sep == line.npos)
			return false;

		std::string key = parseString__(line, 0, sep);

		if (key.empty())
			return false;

		std::string val = parseString__(line, sep + 1, line.size());

		if (val.empty())
			return false;

		std::cout << "-" << key << "=" << val << "-" << '\n';

		data_.emplace_back(std::move(key), std::move(val));

		return true;
	}

private:
	static void parseComment__(std::string &line){
		if (line.find(COMMENT) != line.npos)
			line.erase(line.find(COMMENT));
	}

	static std::string parseString__(const std::string &line, size_t const start, size_t const end){
		std::string s = line.substr(start, end);
		trim__(s);

		return s;
	}

	static void trim__(std::string &s){
		constexpr const char *TRIM_CHARACTERS = " \t";

		s.erase(0, s.find_first_not_of(TRIM_CHARACTERS));
		s.erase(s.find_last_not_of(TRIM_CHARACTERS) + 1);
	}

private:
	/* static */
	std::string		null_;

	std::vector<INIPair>	data_;
};



int main(){
	INIFile ini("ini.txt");

	std::cout << ini.get("os") << '\n';
	std::cout << ini.getInt<uint16_t>("unit") << '\n';
	std::cout << ini.getInt<uint16_t>("unit2", 5) << '\n';
}

