#include <cstdint>
#include <iostream>
#include <iomanip>

#include "inifile.h"

class MyOptions{
public:
	uint16_t	immutable		= 1;
	std::string	db_path;

	std::nullptr_t	host			= nullptr;
	uint16_t	port			= 2000;
	uint32_t	timeout			= 30;

	uint32_t	max_clients		= 512;
	size_t		max_memlist_size	= 100;

private:
	enum class Option{
		NONE			,

		immutable		,
		db_path   	    	,

		host			,
		port			,
		timeout			,

		max_clients		,
		max_memlist_size
	};

	constexpr static size_t BUCKETS		= 38;

	using OptionsMap = StringMap<Option, BUCKETS, Option::NONE>;

	constexpr static OptionsMap map = {
			{ "immutable",		Option::immutable		},
                        { "db_path",		Option::db_path			},

                        { "host",		Option::host			},
                        { "port",		Option::port			},
                        { "timeout",		Option::timeout			},

                        { "max_clients",	Option::max_clients		},
                        { "max_memlist_size",	Option::max_memlist_size	}
	};

	static_assert( map, "Collision, change number of buckets to something else" );

public:
	void operator()(const StringRef &name, const StringRef &value){
		const auto opt = map[name];

		switch(opt){
		case Option::immutable		: return assign_(immutable,		value);
		case Option::db_path		: return assign_(db_path,  	    	value);

		case Option::host		: return assign_(host,			value);
		case Option::port		: return assign_(port,			value);
		case Option::timeout		: return assign_(timeout,		value);

		case Option::max_clients	: return assign_(max_clients,		value);
		case Option::max_memlist_size	: return assign_(max_memlist_size,	value);

		default				: return;
		}
	}

	static void print(){
		print__("immutable",		"1",	"Start immutable server (1/0)"		);
		print__("db_path",		"-",	"Path to database"			);

		print__("host",			"-",	"TCP host to listen (not working)"	);
		print__("port",			"2000",	"TCP port to listen"			);
		print__("timeout",		"30",	"Connection timeout in seconds"		);

		print__("max_clients",		"512",	"Max Clients"				);
		print__("max_memlist_size",	"100",	"Max size of memlist in MB"		);
	}

private:
	template<class T>
	static void assign_(T &param, const StringRef &value){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");

		auto const default_value = param;

		param = stou_safe<T>(value, default_value);
	}

	static void assign_(std::string &param, const StringRef &value){
		param = value;
	}

	static void assign_(std::nullptr_t, const StringRef &){
		/* nada */
	}

private:
	static void print__(const StringRef &name, const char *def, const char *description){
		std::cout
			<< '\t'
			<< std::setw(20) << std::left << name
			<< std::setw( 8) << std::left << def
			<< description
			<< '\n'
		;
	}
};

constexpr MyOptions::OptionsMap MyOptions::map;

