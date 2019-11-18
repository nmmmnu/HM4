#include <cstdint>
#include <iostream>
#include <iomanip>

#include "inifile.h"
#include "mystring.h"

class MyOptions{
public:
	uint16_t	immutable		= 1;
	std::string	db_path;

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
		case hash("db_path"		)	: return assign_(db_path,  	    	value);

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
		print__("immutable",		"1",	"Start mutable = 0, immutable = 1"		);
		print__("db_path",		"-",	"Path to database"				);

		print__("host",			"-",	"TCP host to listen (not working)"		);
		print__("port",			"2000",	"TCP port to listen"				);
		print__("timeout",		"30",	"Connection timeout in seconds"			);

		print__("max_clients",		"512",	"Max Clients"					);
		print__("max_memlist_size",	"128",	"Max size of memlist in MB"			);
		print__("max_memlist_arena",	"0",	"Max size of memlist AllocatorArena in MB"	);
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

private:
	static void print__(std::string_view const name, const char *def, const char *description){
		std::cout
			<< '\t'
			<< std::setw(20) << std::left << name
			<< std::setw( 0) << '=' << ' '
			<< std::setw( 8) << std::left << def
			<< std::setw( 0) << ';' << ' '
			<< description
			<< '\n'
		;
	}
};

