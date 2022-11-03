#ifndef _KEY_VALUE_COMMANDS_BASE_H
#define _KEY_VALUE_COMMANDS_BASE_H

#include "myspan.h"
#include "iobuffer.h"
#include "../workerdefs.h"

#include <memory>
#include <string>
#include <variant>

#include "staticvector.h"
#include "mystring.h"

namespace net::worker::commands{



	enum class Status{
		OK,
		ERROR,

		DISCONNECT,
		SHUTDOWN
	};



	struct OutputBlob{
		constexpr static size_t ContainerSize	= 65'536;
		using Container = StaticVector<std::string_view,ContainerSize>;

		constexpr static size_t BufferKeySize	= hm4::PairConf::MAX_KEY_SIZE + 16;

		OutputBlob(){
			container.reserve(ContainerSize);
		}

		Container			container;
		std::array<char, BufferKeySize>	buffer_key;
		std::string			string;
	};



	struct Result{
		using ResultDataSpan = MySpan<std::string_view, MySpanConstructor::EXPLICIT>;

		using ResultData = std::variant<
			std::nullptr_t		,
			bool			,
			int64_t			,
			uint64_t		,
			std::string_view	,
			ResultDataSpan
		>;



		Status		status	= Status::OK;
		ResultData	data	= std::nullptr_t{};



		constexpr static Result ok(ResultData &&data = nullptr){
			return { Status::OK, std::move(data) };
		}

		constexpr static Result ok_0(){
			return { Status::OK, "0" };
		}

		constexpr static Result ok_1(){
			return { Status::OK, "1" };
		}

		template<class Container>
		constexpr static Result ok_container(const Container &container){
			return ok(
				ResultDataSpan{ container }
			);
		}

		constexpr static Result error(){
			return { Status::ERROR, nullptr };
		}
	};



	using ParamContainer  = MySpan<std::string_view>;



	template<class DBAdapter>
	struct Base{
		constexpr static bool mut		= false;

		virtual ~Base() = default;

		virtual Result operator()(ParamContainer const &params, DBAdapter &db, OutputBlob &) const = 0;
	};



	template<
		template<class>  class Cmd,
		class DBAdapter,
		class RegisterPack
	>
	void registerCommand(RegisterPack &pack){
		using Command		= Cmd <DBAdapter>;
		using CommandBase	= Base<DBAdapter>;

		static_assert(std::is_base_of_v<CommandBase, Command>, "Command not seems to be a command");

		if constexpr(Command::mut == false || Command::mut == DBAdapter::MUTABLE ){
			auto &up = pack.storage.emplace_back(std::make_unique<Command>());

			const CommandBase *p = up.get();

			for(auto const &key : Command::cmd)
				pack.map.emplace(key, p);
		}
	}



	template<
		class DBAdapter,
		class RegisterPack,
		template<class> typename... Commands
	>
	void registerCommands(RegisterPack &pack){
		( registerCommand<Commands, DBAdapter>(pack), ... );
	}



}


#endif

