#include "base.h"



namespace net::worker::commands::System{



	template<class DBAdapter>
	struct SELECT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "select";
		constexpr inline static std::string_view cmd[]	= {
			"select",	"SELECT"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok();
		}
	};

	template<class DBAdapter>
	struct TYPE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "type";
		constexpr inline static std::string_view cmd[]	= {
			"type",	"TYPE"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok(
				"string"
			);
		}
	};

	template<class DBAdapter>
	struct TOUCH : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "touch";
		constexpr inline static std::string_view cmd[]	= {
			"touch",	"TOUCH"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return Result::ok(
				1
			);
		}
	};

	template<class DBAdapter>
	struct EXIT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "exit";
		constexpr inline static std::string_view cmd[]	= {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return { Status::DISCONNECT, nullptr };
		}
	};

	template<class DBAdapter>
	struct SHUTDOWN : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "shutdown";
		constexpr inline static std::string_view cmd[]	= {
			"shutdown",	"SHUTDOWN"
		};

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			return { Status::SHUTDOWN, nullptr };
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "system";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				SELECT	,
				TYPE	,
				TOUCH	,
				EXIT	,
				SHUTDOWN
			>(pack);
		}
	};



} // namespace


