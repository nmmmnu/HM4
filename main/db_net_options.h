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

public:
	void operator()(const StringRef &name, const StringRef &value){
		switch( name.hash() ){
		case "immutable"_sr.hash()		: return assign_(immutable,		value);
		case "db_path"_sr.hash()		: return assign_(db_path,  	    	value);

		case "host"_sr.hash()			: return assign_(host,			value);
		case "port"_sr.hash()			: return assign_(port,			value);
		case "timeout"_sr.hash()		: return assign_(timeout,		value);

		case "max_clients"_sr.hash()		: return assign_(max_clients,		value);
		case "max_memlist_size"_sr.hash()	: return assign_(max_memlist_size,	value);

		default					: return;
		}
	}

	static void print(){
		print__("immutable",		"1",	"Start mutable = 0, immutable = 1, single = 2"	);
		print__("db_path",		"-",	"Path to database"				);

		print__("host",			"-",	"TCP host to listen (not working)"		);
		print__("port",			"2000",	"TCP port to listen"				);
		print__("timeout",		"30",	"Connection timeout in seconds"			);

		print__("max_clients",		"512",	"Max Clients"					);
		print__("max_memlist_size",	"100",	"Max size of memlist in MB"			);
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

