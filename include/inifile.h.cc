#include <cassert>
#include "stou_safe.h"

namespace {
namespace inifile_impl_{

	template<class PROCESSOR>
	class INILineParser{
	private:
		constexpr static const char *SEPARATOR	= "=";
		constexpr static const char *REM	= ";#";
		constexpr static const char *SPACE	= " \t\r\n";

	public:
		INILineParser(const std::string &line, PROCESSOR &proc) :
						line(line), proc(proc){}

		bool operator()(){
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
			bool const r = is_separator__(line[pos]);
			++pos;
			return r;
		}

	private:
		static constexpr bool is_space__(char const c){
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

		static constexpr bool is_separator__(char const c){
			return	c == SEPARATOR[0]
			;
		}

		static constexpr bool is_word__(char const c){
			return	c != 0			&&
				! is_separator__(c)	&&
				! is_comment__(c)
			;
		}

	private:
		const std::string	&line;
		PROCESSOR		&proc;
		size_t			pos = 0;

	};

} // namespace inifile_impl_
} // namespace


// ===================================


template<class PROCESSOR>
bool readINIFile(std::istream &file, PROCESSOR &processor){
	using MyINILineParser = inifile_impl_::INILineParser<PROCESSOR>;

	std::string line;

	while (std::getline(file, line)){
		MyINILineParser pp{ line, processor };

		pp();
	}

	return true;
}


template<class PROCESSOR>
bool readINIFile(const StringRef &filename, PROCESSOR &processor){
	assert(!filename.empty());

	std::ifstream file;
	file.open(filename);

	if (!file)
		return false;

	return readINIFile(file, processor);
}


