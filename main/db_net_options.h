#include <cstdint>
#include <array>
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
	enum class Option{
		immutable		,
		db_path   	    	,

		host			,
		port			,
		timeout			,

		max_clients		,
		max_memlist_size
	};

private:
	struct OptPair{
		StringRef	str;
		Option		opt;
		const char	*def;
		const char	*descr;

		bool operator == (const StringRef &s) const{
			return str == s;
		}
	};

private:
	constexpr static size_t ARRAY_SIZE = 7;

	using OptionsArray = std::array<OptPair, ARRAY_SIZE>;

	constexpr static OptionsArray options{
		OptPair{ "immutable",		Option::immutable,		"1",	"Start immutable server (1/0)"		},
		OptPair{ "db_path",		Option::db_path,  	    	"-",	"Path to database"			},

		OptPair{ "host",		Option::host,			"-",	"TCP host to listen (not working)"	},
		OptPair{ "port",		Option::port,			"2000",	"TCP port to listen"			},
		OptPair{ "timeout",		Option::timeout,		"30",	"Connection timeout in seconds"		},

		OptPair{ "max_clients",		Option::max_clients,		"512",	"Max Clients"				},
		OptPair{ "max_memlist_size",	Option::max_memlist_size,	"100",	"Max size of memlist in MB"		}
	};

private:
	void assignOption_(const Option opt, const StringRef &value){
		switch(opt){
		case Option::immutable		: return assign_(immutable,		value);
		case Option::db_path		: return assign_(db_path,  	    	value);

		case Option::host		: return assign_(host,			value);
		case Option::port		: return assign_(port,			value);
		case Option::timeout		: return assign_(timeout,		value);

		case Option::max_clients	: return assign_(max_clients,		value);
		case Option::max_memlist_size	: return assign_(max_memlist_size,	value);
		}
	}

public:
	void operator()(const StringRef &name, const StringRef &value){
		const auto &it = std::find(options.begin(), options.end(), name);

		if (it != options.end())
			return assignOption_(it->opt, value);
	}

	static void print(){
		for(const auto &o : options)
			print__(o.str, o.def, o.descr);
	}

private:
	template<class T>
	static void assign_(T &param, const StringRef &value){
		static_assert(std::is_unsigned<T>::value, "T must be unsigned type");

		auto const default_value = param;

		param = stou<T>(value, default_value);
	}

	static void assign_(std::string &param, const StringRef &value){
		param = value;
	}

	static void assign_(nullptr_t, const StringRef &){
		/* nada */
	}

private:
	static void print__(const StringRef &name, const char *def, const char *description){
		std::cout
			<< '\t'
			<< std::setw(20) << std::left << name
			<< std::setw(8) << std::left << def
			<< description
			<< '\n'
		;
	}
};

constexpr MyOptions::OptionsArray MyOptions::options;
