#include <cstdint>

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "inifile.h"
#include "mystring.h"

namespace impl_{
	template <typename Def>
	void put(std::string_view const name, Def const &def, std::string_view const description){
		fmt::print("\t{0:25} = {1:>14} ; {2}\n", name, def, description);
	}

	inline void put(std::string_view const name, std::string_view const description){
		put(name, "-", description);
	}
}



class MyOptions{
public:
	std::string	db_path;

	std::nullptr_t	compaction_percent;

	uint64_t	compaction_min_records	= 128;
	uint64_t	compaction_max_records	= 128'000'000;

	uint16_t	compaction_max_tables	= 8;

	std::string	rename_from;
	std::string	rename_to;

public:
	void operator()(std::string_view const name, std::string_view const value){
		switch( hash(name) ){
		case hash("db_path"			)	: return assign_(db_path,			value);

		case hash("compaction_percent"		)	: return assign_(compaction_percent,		value);

		case hash("compaction_min_records"	)	: return assign_(compaction_min_records,	value);
		case hash("compaction_max_records"	)	: return assign_(compaction_max_records,	value);

		case hash("compaction_max_tables"	)	: return assign_(compaction_max_tables,		value);

		case hash("rename_from"			)	: return assign_(rename_from,			value);
		case hash("rename_to"			)	: return assign_(rename_to,			value);

		default						: return;
		}
	}

	static void print(){
		MyOptions{}.printOptions();
	}

	void printOptions() const{
		using impl_::put;

		put("db_path",						"Path to database"							);

		put("compaction_percent",	compaction_percent,	"not used yet. hard set to 25%"						);

		put("compaction_min_records",	compaction_min_records,	"Tables with less than min_records are always compacted, 0 = disabled"	);
		put("compaction_max_records",	compaction_max_records,	"Tables with more than min_records are *NOT*  compacted, 0 = disabled"	);

		put("compaction_max_tables",	compaction_max_tables,	"Max tables to compact at once                         , 0 = disabled"	);

		put("rename_from",					"Rename from"								);
		put("rename_to",					"Rename to"								);
	}

private:
	template<class T>
	static void assign_(T &param, std::string_view const value){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");

		auto const default_value = param;

		param = from_string<T>(value, default_value);
	}

	static void assign_(std::string &param, std::string_view const value){
		param = value;
	}

	[[maybe_unused]]
	static void assign_(std::nullptr_t, std::string_view){
		/* nada */
	}
};

