#include <cstdint>

#include "stringref.h"
#include "stou_safe.h"

class MyOptions{
public:
	uint8_t		immutable		= 1;		// Start immutable server (1/0)
	std::string	db_path;				// Path to database

	nullptr_t	host			= nullptr;	// TCP host to listen (not working)
	uint16_t	port			= 2000;		// TCP port to listen
	uint32_t	timeout			= 30;		// Connection timeout in seconds

	uint32_t	max_clients		= 512;		// Max Clients
	size_t		max_memlist_size	= 100;		// Max size of memlist in MB

public:
	void operator()(const StringRef &name, const StringRef &val){
		do_(name, val, "immutable",		immutable		);
		do_(name, val, "db_path",		db_path			);

		do_(name, val, "host",			host			);
		do_(name, val, "port",			port			);
		do_(name, val, "timeout",		timeout			);

		do_(name, val, "max_clients",		max_clients		);
		do_(name, val, "max_memlist_size",	max_memlist_size	);
	}

private:
	template<class T>
	static void do_(const StringRef &name, const StringRef &value, const char *opt_name, T &opt_value){
		if (name == opt_name)
			assign_(value, opt_value);
	}

private:
	template<class T>
	static void assign_(const StringRef &value, T &opt_value){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");
		auto const default_value = opt_value;
		opt_value = stou<T>(value, default_value);
	}

	static void assign_(const StringRef &value, std::string &opt_value){
		opt_value = value;
	}

	static void assign_(const StringRef &, nullptr_t){
		/* nada */
	}
};

