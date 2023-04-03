#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "version.h"
#include "myprocess.h"	// get PID
#include "pair.h"
#include "logger.h"

template<class List_, class CommandSave=std::nullptr_t, class CommandReload=std::nullptr_t>
struct ListDBAdapter{
	using List = List_;

	constexpr static bool MUTABLE 			= ! std::is_const_v<List>;
	constexpr static bool DELETE_USE_TOMBSTONES	= true;
	constexpr static auto LOG_LEVEL			= LogLevel::NOTICE;

	constexpr static inline std::string_view SEPARATOR = "~";

public:
	ListDBAdapter(List &list, CommandSave &cmdSave, CommandReload &cmdReload) :
				list_(list),
				cmdSave_	(& cmdSave	),
				cmdReload_	(& cmdReload	){}

public:
	// System Methods

	template<size_t N>
	std::string_view info(std::array<char, N> &str) const{
		to_string_buffer_t buffer[8];

		if constexpr(!MUTABLE){
			int i = 0;

			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wsequence-point"

			return concatenateBuffer(
				str,

				"# Server"											"\n",
				"Version          : ", version()					,			"\n",

			"\n"	"# Pair Limits"											"\n",
				"Max Key Size     : ", to_string(hm4::PairConf::MAX_KEY_SIZE		,	buffer[i++]),	"\n",
				"Max Val Size     : ", to_string(hm4::PairConf::MAX_VAL_SIZE		,	buffer[i++]),	"\n",

			"\n"	"# Keys"											"\n",
				"Mutable          : ", MUTABLE ? "Yes" : "No"				,			"\n",
				"Keys (estimated) : ", to_string(list_.size()				,	buffer[i++]),	"\n",
				"Size             : ", to_string(list_.bytes()				,	buffer[i++]),	"\n",

			"\n"	"# System"											"\n",
				"PID              : ", to_string(getProcessID()				,	buffer[i++]),	"\n"
			);

			#pragma GCC diagnostic pop

		}else{
			auto mem_format = [](uint64_t const a, to_string_buffer_t &buffer) -> std::string_view{
				if (a == std::numeric_limits<std::size_t>::max())
					return "n/a";
				else
					return to_string(a, buffer);
			};

			int i = 0;

			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wsequence-point"

			return concatenateBuffer(
				str,

				"# Server"											"\n",
				"Version          : ", version()					,			"\n",

			"\n"	"# Pair Limits"											"\n",
				"Max Key Size     : ", to_string(hm4::PairConf::MAX_KEY_SIZE		,	buffer[i++]),	"\n",
				"Max Val Size     : ", to_string(hm4::PairConf::MAX_VAL_SIZE		,	buffer[i++]),	"\n",

			"\n"	"# Keys"											"\n",
				"Mutable          : ", MUTABLE ? "Yes" : "No"				,			"\n",
				"Keys (estimated) : ", to_string(list_.size()				,	buffer[i++]),	"\n",
				"Size             : ", to_string(list_.bytes()				,	buffer[i++]),	"\n",
				"Mutable Keys     : ", to_string(list_.mutable_size()			,	buffer[i++]),	"\n",

			"\n"	"# Allocator"											"\n",
				"Allocator        : ", list_.getAllocator().getName()			,			"\n",
				"Allocator Free   : ", mem_format(list_.getAllocator().getFreeMemory()	,	buffer[i++]),	"\n",
				"Allocator Used   : ", mem_format(list_.getAllocator().getUsedMemory()	,	buffer[i++]),	"\n",

			"\n"	"# System"											"\n",
				"PID              : ", to_string(getProcessID()				,	buffer[i++]),	"\n"
			);

			#pragma GCC diagnostic pop
		}
	}

	constexpr static std::string_view version(){
		return hm4::version::str;
	}

	auto save(){
		return invokeCommand__(cmdSave_);
	}

	auto reload(){
		return invokeCommand__(cmdReload_);
	}

public:
	// List Access Methods

	auto const &operator*() const{
		return list_;
	}

	auto const *operator->() const{
		return &list_;
	}

	auto &operator*(){
		return list_;
	}

	auto *operator->(){
		return &list_;
	}

private:
/*
	static void logHint__(const char *msg){
		log__<LOG_LEVEL>(msg, "Bypassing list");
	}

	template<typename T>
	static T logHint__(const char *msg, T t){
		logHint__(msg);
		return t;
	}
*/

	template<class Command>
	static bool invokeCommand__(Command *cmd){
		if constexpr(std::is_same_v<Command,std::nullptr_t>)
			return false;
		else
			return cmd && cmd->command();
	}

private:
	List		&list_;
	CommandSave	*cmdSave_	= nullptr;
	CommandReload	*cmdReload_	= nullptr;
};

#endif

