#include <cstdint>

#define FMT_HEADER_ONLY
#include "fmt/printf.h"

#include "inifile.h"
#include "mystring.h"

namespace impl_{
	template <typename Def>
	void put(std::string_view const name, Def const &def, std::string_view const description){
		fmt::print("\t{0:20} = {1:>8} ; {2}\n", name, def, description);
	}

	inline void put(std::string_view const name, std::string_view const description){
		put(name, "-", description);
	}
}



class MyOptions{
public:
	uint16_t	immutable		= 1;
	std::string	db_path;

	std::string	binlog_path1;
	std::string	binlog_path2;
	uint16_t	binlog_fsync		= 0;

	std::nullptr_t	host			= nullptr;
	uint16_t	port			= 2000;
	uint32_t	timeout			= 30;

	uint32_t	max_clients		= 512;
	size_t		max_memlist_size	= 128;
	size_t		max_memlist_arena	= 0;

public:
	void operator()(std::string_view const name, std::string_view const value){
		switch( hash(name) ){
		case hash("immutable"		)	: return assign_(immutable,		value);
		case hash("db_path"		)	: return assign_(db_path,		value);

		case hash("binlog_path"		)	: return assign_(binlog_path1,		value);
		case hash("binlog_path1"	)	: return assign_(binlog_path1,		value);
		case hash("binlog_path2"	)	: return assign_(binlog_path2,		value);
		case hash("binlog_fsync"	)	: return assign_(binlog_fsync,		value);

		case hash("host"		)	: return assign_(host,			value);
		case hash("port"		)	: return assign_(port,			value);
		case hash("timeout"		)	: return assign_(timeout,		value);

		case hash("max_clients"		)	: return assign_(max_clients,		value);
		case hash("max_memlist_size"	)	: return assign_(max_memlist_size,	value);
		case hash("max_memlist_arena"	)	: return assign_(max_memlist_arena,	value);

		default					: return;
		}
	}

	static void print(){
		MyOptions{}.printOptions();
	}

	void printOptions() const{
		using impl_::put;

		put("immutable",		immutable,		"Start mutable = 0, immutable = 1"		);
		put("db_path",						"Path to database"				);

		put("binlog_path",					"Path to binlog, empty for none"		);
		put("binlog_path1",					"Path to binlog, empty for none"		);
		put("binlog_path2",					"Path to binlog, empty for none"		);
		put("binlog_fsync",		binlog_fsync,		"fsync() binlog - none = 0, yes = 1"		);

		put("host",						"TCP host to listen (not working)"		);
		put("port",			port,			"TCP port to listen"				);
		put("timeout",			timeout,		"Connection timeout in seconds"			);

		put("max_clients",		max_clients,		"Max Clients"					);
		put("max_memlist_size",		max_memlist_size,	"Max size of memlist in MB"			);
		put("max_memlist_arena",	max_memlist_arena,	"Max size of memlist AllocatorArena in MB"	);
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

	static void assign_(std::nullptr_t, std::string_view){
		/* nada */
	}
};

