#include <cstdint>
#include <iostream>
#include <iomanip>

#include "inifile.h"

class MyOptions{
public:
	uint8_t		immutable		= 1;
	std::string	db_path;

	nullptr_t	host			= nullptr;
	uint16_t	port			= 2000;
	uint32_t	timeout			= 30;

	uint32_t	max_clients		= 512;
	size_t		max_memlist_size	= 100;

private:
	constexpr static const char *OPT_immutable		= "immutable"		;
	constexpr static const char *OPT_db_path        	= "db_path"		;

	constexpr static const char *OPT_host			= "host"		;
	constexpr static const char *OPT_port			= "port"		;
	constexpr static const char *OPT_timeout		= "timeout"		;

	constexpr static const char *OPT_max_clients		= "max_clients"		;
	constexpr static const char *OPT_max_memlist_size	= "max_memlist_size"	;

public:
	void operator()(const StringRef &name, const StringRef &val){
		const INIFileAssignHelper ini{ name, val };

		ini.setOption(OPT_immutable,		immutable		);
		ini.setOption(OPT_db_path,		db_path			);

		ini.setOption(OPT_host,			host			);
		ini.setOption(OPT_port,			port			);
		ini.setOption(OPT_timeout,		timeout			);

		ini.setOption(OPT_max_clients,		max_clients		);
		ini.setOption(OPT_max_memlist_size,	max_memlist_size	);
	}

	static void print(){
		const MyOptions opt;

		print__(OPT_immutable,		opt.immutable,		"Start immutable server (1/0)"		);
		print__(OPT_db_path,		opt.db_path,		"Path to database"			);

		print__(OPT_host,		opt.host,		"TCP host to listen (not working)"	);
		print__(OPT_port,		opt.port,		"TCP port to listen"			);
		print__(OPT_timeout,		opt.timeout,		"Connection timeout in seconds"		);

		print__(OPT_max_clients,	opt.max_clients,	"Max Clients"				);
		print__(OPT_max_memlist_size,	opt.max_memlist_size,	"Max size of memlist in MB"		);
	}

private:
	template<class T>
	static void print__(const char *name, const T &value, const char *description){
		std::cout
			<< '\t'
			<< std::setw(20) << std::left << name
			<< std::setw(6) << std::left << value
			<< description
			<< '\n'
		;
	}

	static void print__(const char *name, uint8_t const value, const char *description){
		print__(name, int{ value }, description);
	}

	static void print__(const char *name, nullptr_t, const char *description){
		print__(name, "n/a", description);
	}
};

