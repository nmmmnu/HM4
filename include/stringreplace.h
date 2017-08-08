#ifndef MY_STR_REPLACE_H_
#define MY_STR_REPLACE_H_

#include "stringref.h"

class StringReplace{
public:
	std::string &operator()(std::string &s, const StringRef &find, const StringRef &replace) const{
		return replace_(s, find, replace);
	}

	std::string &operator()(std::string &s, char const find, const StringRef &replace) const{
		return replace_(s, find, replace);
	}

private:
	template<class STR>
	static std::string &replace_(std::string &s, const STR &find, const StringRef &replace){
		auto const findsize = strlen_(find);

		if (findsize == 0)
			return s;

		size_t const pos = s.find(find);

		if (pos == std::string::npos)
			return s;

		return s.replace(pos, findsize, replace.data(), replace.size());
	}

private:
	constexpr static size_t strlen_(const StringRef &s){
		return s.size();
	}

	constexpr static size_t strlen_(char const){
		return 1;
	}
};

// ==============================

class StringReplaceCopy{
public:
	std::string operator()(std::string s, const StringRef &find, const StringRef &replace) const{
		return replace_(s, find, replace);
	}

	std::string operator()(std::string s, char const find, const StringRef &replace) const{
		return replace_(s, find, replace);
	}

private:
	StringReplace replace_;
};

#endif

