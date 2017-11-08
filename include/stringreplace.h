#ifndef MY_STR_REPLACE_H_
#define MY_STR_REPLACE_H_

#include "stringref.h"

namespace StringReplace{

	namespace string_replace_impl_{
		constexpr static size_t strlen_(const StringRef &s){
			return s.size();
		}

		constexpr static size_t strlen_(char const){
			return 1;
		}

		template<class STR>
		std::string &replace_(std::string &s, const STR &find, const StringRef &replace){
			auto const findsize = strlen_(find);

			if (findsize == 0)
				return s;

			size_t const pos = s.find(find);

			if (pos == std::string::npos)
				return s;

			return s.replace(pos, findsize, replace.data(), replace.size());
		}

	}

	std::string &replace(std::string &s, const StringRef &find, const StringRef &repl){
		using namespace string_replace_impl_;

		return replace_(s, find, repl);
	}

	std::string &replace(std::string &s, char const find, const StringRef &repl){
		using namespace string_replace_impl_;

		return replace_(s, find, repl);
	}

	std::string replaceByCopy(std::string s, const StringRef &find, const StringRef &repl){
		return replace(s, find, repl);
	}

	std::string replaceByCopy(std::string s, char const find, const StringRef &repl){
		return replace(s, find, repl);
	}
} // namespace

#endif

