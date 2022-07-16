#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "iobuffer.h"
#include "worker/keyvalueerror.h"

#include <memory>

namespace net::worker::commands{



	template<class Protocol, class DBAdapter>
	struct Base{
		constexpr static bool mut		= false;

		virtual ~Base() = default;
		virtual WorkerStatus operator()(Protocol &protocol, typename Protocol::StringVector const &params, DBAdapter &db, IOBuffer &buffer) const = 0;
	};



	template<
		template<class, class>  class Cmd,
		class Protocol,
		class DBAdapter,
		class Storage,
		class Map
	>
	void registerCmd(Storage &s, Map &m){
		using Command		= Cmd <Protocol, DBAdapter>;
		using CommandBase	= Base<Protocol, DBAdapter>;

		static_assert(std::is_base_of_v<CommandBase, Command>, "Command not seems to be a command");

		if constexpr(Command::mut == false || Command::mut == DBAdapter::MUTABLE ){
			const auto &up = s.emplace_back(std::make_unique<Command>());

			const CommandBase *p = up.get();

			for(auto const &key : Command::cmd)
				m.emplace(key, p);
		}
	}



	template<
		class Protocol,
		class DBAdapter,
		class Storage,
		class Map,
		template<class, class> typename... Commands
	>
	void registerCommands(Storage &s, Map &m){
		( registerCmd<Commands, Protocol, DBAdapter>(s, m), ... );
	}



}


#endif

