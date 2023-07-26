#include <cstdint>

#define FMT_HEADER_ONLY
#include "fmt/core.h"

#include "inifile.h"
#include "mystring.h"

namespace impl_{
	template <typename Def>
	void put(std::string_view const name, Def const &def, std::string_view const description){
		fmt::print("\t{0:30} = {1:>14} ; {2}\n", name, def, description);
	}

	inline void put(std::string_view const name, std::string_view const description){
		put(name, "-", description);
	}
}



class MyOptions{
public:
	uint16_t	immutable			= 1;
	std::string	db_path;

	std::string	binlog_path1;
	std::string	binlog_path2;
	uint16_t	binlog_fsync			= 0;

	std::nullptr_t	host				= nullptr;
	uint16_t	port				= 2000;
	uint32_t	timeout				= 60 * 5;

	uint16_t	tcp_backlog			= 0;
	uint16_t	tcp_reuseport			= 0;

	uint32_t	max_clients			= 512;
	uint32_t	min_spare_pool			= 32;
	uint32_t	max_spare_pool			= 64;

	size_t		buffer_capacity			= 4096;

	size_t		max_memlist_arena		= 0;
	uint16_t	map_memlist_arena		= 1;

	uint32_t	crontab_reload			= 90;
	uint32_t	crontab_table_maintainance	= 90;
	uint32_t	crontab_server_info		= 90;

	uint16_t	log_level			= 4;

	constexpr static
	uint32_t	crontab_min_time		= 15;

public:
	void operator()(std::string_view const name, std::string_view const value){
		switch( hash(name) ){
		case hash("immutable"			)	: return assign_(immutable,			value);
		case hash("db_path"			)	: return assign_(db_path,			value);

	//	case hash("binlog_path"			)	: return assign_(binlog_path1,			value);
		case hash("binlog_path1"		)	: return assign_(binlog_path1,			value);
		case hash("binlog_path2"		)	: return assign_(binlog_path2,			value);
		case hash("binlog_fsync"		)	: return assign_(binlog_fsync,			value);

		case hash("host"			)	: return assign_(host,				value);
		case hash("port"			)	: return assign_(port,				value);
		case hash("timeout"			)	: return assign_(timeout,			value);

		case hash("tcp_backlog"			)	: return assign_(tcp_backlog,			value);
		case hash("tcp_reuseport"		)	: return assign_(tcp_reuseport,			value);

		case hash("max_clients"			)	: return assign_(max_clients,			value);
		case hash("min_spare_pool"		)	: return assign_(min_spare_pool,		value);
		case hash("max_spare_pool"		)	: return assign_(max_spare_pool,		value);

		case hash("buffer_capacity"		)	: return assign_(buffer_capacity,		value);

		case hash("max_memlist_arena"		)	: return assign_(max_memlist_arena,		value);
		case hash("map_memlist_arena"		)	: return assign_(map_memlist_arena,		value);

		case hash("crontab_reload"		)	: return assign_(crontab_reload,		value);
		case hash("crontab_table_maintainance"	)	: return assign_(crontab_table_maintainance,	value);
		case hash("crontab_server_info"		)	: return assign_(crontab_server_info,		value);

		case hash("log_level"			)	: return assign_(log_level,			value);

		default					: return;
		}
	}

	static void print(){
		MyOptions{}.printOptions();
	}

	void printOptions() const{
		using impl_::put;

		put("immutable",			immutable,			"Start mutable=0, immutable=1"							);
		put("db_path",								"Path to database"								);

	//	put("binlog_path",							"Path to binlog, empty for none"						);
		put("binlog_path1",							"Path to binlog, empty for none"						);
		put("binlog_path2",							"Path to binlog, empty for none"						);
		put("binlog_fsync",			binlog_fsync,			"fsync() binlog - none=0, yes=1"						);

		put("host",								"TCP host to listen (not working)"						);
		put("port",				port,				"TCP port to listen"								);
		put("timeout",				timeout,			"Connection timeout in seconds"							);

		put("tcp_backlog",			tcp_backlog,			"TCP backlog"									);
		put("tcp_reuseport",			tcp_reuseport,			"TCP Activate SO_REUSEPORT"							);

		put("max_clients",			max_clients,			"Max Clients"									);
		put("min_spare_pool",			min_spare_pool,			"Min Spare Pool Buffers"							);
		put("max_spare_pool",			max_spare_pool,			"Max Spare Pool Buffers"							);

		put("buffer_capacity",			buffer_capacity,		"Initial size of Spare Pool Buffers"						);

		put("max_memlist_arena",		max_memlist_arena,		"Max size of memlist AllocatorArena in MB"					);
		put("map_memlist_arena",		max_memlist_arena,		"Map all virtual memory pages used from AllocatorArena (Linux)"			);

		put("crontab_reload",			crontab_reload,			"crontab - reload every XX seconds, 0 disabled, min 15 sec"			);
		put("crontab_table_maintainance",	crontab_table_maintainance,	"crontab - table maintainance every XX seconds, 0 disabled, min 15 sec"		);
		put("crontab_server_info",		crontab_server_info,		"crontab - server info every XX seconds, 0 disabled, min 15 sec"		);

		put("log_level",			log_level,			"server log level start=0, fatal=1, err=2, warn=3, notice=4, debug=5"		);
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

