#ifndef MY_STR_REPLACE_H_
#define MY_STR_REPLACE_H_

#include <string>
#include <string_view>

namespace StringReplace{

	namespace stringreplace_impl_{

		constexpr inline size_t strlen_(std::string_view const s){
			return s.size();
		}

		constexpr inline size_t strlen_(char){
			return 1;
		}

		template<class STR>
		std::string &replace_(std::string &s, const STR &find, std::string_view const replace){
			auto const findsize = strlen_(find);

			if (findsize == 0)
				return s;

			size_t const pos = s.find(find);

			if (pos == std::string::npos)
				return s;

			return s.replace(pos, findsize, replace.data(), replace.size());
		}

	} // namespace stringreplace_impl_

	std::string &replace(std::string &s, std::string_view const find, std::string_view const repl){
		using namespace stringreplace_impl_;

		return replace_(s, find, repl);
	}

	std::string &replace(std::string &s, char find, std::string_view const repl){
		using namespace stringreplace_impl_;

		return replace_(s, find, repl);
	}

	std::string replaceByCopy(std::string s, std::string_view const find, std::string_view const repl){
		return replace(s, find, repl);
	}

	std::string replaceByCopy(std::string s, char find, std::string_view const repl){
		return replace(s, find, repl);
	}
} // namespace

#endif

