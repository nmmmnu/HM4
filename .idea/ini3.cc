#include "stringref.h"

class INILineParser{
private:
	constexpr static char EQUAL		= '=';
	constexpr static const char *REM	= ";#";

public:
	INILineParser(const std::string &line) : line(line){}

	template<class PROCESSOR>
	bool operator()(PROCESSOR &proc){
		const StringRef key = readWord_();

		if (!readSeparator_())
			return false;

		const StringRef val = readWord_();

		if (key.empty() || val.empty())
			return false;

		proc(key, val);

		return true;
	}

private:
	void readWhitespace_(){
		// line is zero terminated, so this is safe
		while(is_space__(line[pos]))
			++pos;
	}

	StringRef readWord_(){
		readWhitespace_();

		size_t const start = pos;
		while(is_word__(line[pos]))
			++pos;

		// trim right
		size_t end = pos;
		while(end > start && is_space__(line[end - 1]))
			--end;

		return { & line[start], end - start };
	}

	bool readSeparator_(){
		bool const r = line[pos] == EQUAL;
		++pos;
		return r;
	}

private:
	static constexpr bool is_space__(char const c){
		constexpr const char *SPACE	= " \t\r\n";

		return	c == SPACE[0]	||
			c == SPACE[1]	||
			c == SPACE[2]	||
			c == SPACE[3]
		;
	}

	static constexpr bool is_comment__(char const c){
		return	c == REM[0]	||
			c == REM[1]
		;
	}

	static constexpr bool is_word__(char const c){
		return	c != 0			&&
			c != EQUAL		&&
			! is_comment__(c)
		;
	}

private:
	const std::string	&line;
	size_t			pos = 0;

};

#include <iostream>

struct PrintProcessor{
	void operator()(const StringRef &key, const StringRef &val) const{
		std::cout << '-' << key << '=' << val << '-' << '\n';
	}
};

inline void fix(const std::string &line){
	const PrintProcessor proc;

	INILineParser p{ line };
	p(proc);
}

int main(){
	fix(" msg = Hello World ");
	fix("msg=Hello World");
	fix("msg=Hello World; comment");
	fix("msg=Hello World ; comment");
	fix("; comment");
	fix("msg; comment");
	fix("msg=; comment");
}

