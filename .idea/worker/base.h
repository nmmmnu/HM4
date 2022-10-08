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

#include "arenaallocator.h"

namespace net::worker::commands{



	enum class Status{
		OK,
		ERROR,

		DISCONNECT,
		SHUTDOWN
	};



	struct OutputBlob{
		constexpr static size_t ContainerSize	= 2 * 2048;
		using Container = StaticVector<std::string_view,ContainerSize>;

		constexpr static size_t AllocatorSize	= 128 * 1024;
		using MyArenaAllocator = MyAllocator::ArenaAllocatorStatic<AllocatorSize>;

		static_assert(AllocatorSize > sizeof(Container), "AllocatorSize must be increased");
		static_assert(MyArenaAllocator::need_deallocate() == false, "Use another allocator");

		static_assert(std::is_trivially_destructible_v<Container>, "Container must be POD-like type");

		Container *getContainer(){
			allocator.reset();

			void *p = MyAllocator::allocate<Container>(allocator);

			return new(p) Container{};
		}

		std::string_view getString(std::string_view s){
			char *p = MyAllocator::allocate<char>(allocator, s.size() + 1);

			memcpy(p, s.data(), s.size());

			p[s.size()] = '\0';

			return p;
		}

		MyArenaAllocator	allocator;

		std::string		string_key;
		to_string_buffer_t	std_buffer[1];
	};



	struct Result{
		using ResultData = std::variant<
			std::nullptr_t		,
			bool			,
			int64_t			,
			uint64_t		,
			std::string_view	,
			MySpan<std::string_view, MySpanConstructor::EXPLICIT>
		>;



		Status		status	= Status::OK;
		ResultData	data	= std::nullptr_t{};

		constexpr Result(Status const status, ResultData &&data) : status(status), data(std::move(data)){};

		constexpr static Result ok(ResultData &&data = nullptr){
			return { Status::OK, std::move(data) };
		}

		constexpr static Result ok_container(const OutputBlob::Container &container){
			return ok(
				MySpan<std::string_view, MySpanConstructor::EXPLICIT>{ container }
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

