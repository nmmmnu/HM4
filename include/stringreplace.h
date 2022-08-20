#ifndef MY_STR_REPLACE_H_
#define MY_STR_REPLACE_H_

#include <string>
#include <string_view>

namespace StringReplace{

	std::string &replace(std::string &s, std::string_view const find, std::string_view const replace){
		auto const findsize = find.size();

		if (findsize == 0)
			return s;

		size_t const pos = s.find(find);

		if (pos == std::string::npos)
			return s;

		return s.replace(pos, findsize, replace.data(), replace.size());
	}


	std::string replaceByCopy(std::string_view const sv, std::string_view const find, std::string_view const repl){
		std::string s{ sv };

		replace(s, find, repl);

		return s;
	}

} // namespace

#endif

