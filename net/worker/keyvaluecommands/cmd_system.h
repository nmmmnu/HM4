#include "base.h"



namespace net::worker::commands::System{



	template<class Protocol, class DBAdapter>
	struct EXIT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_status(Status::DISCONNECT);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"exit",		"EXIT",
			"quit",		"QUIT"
		};
	};

	template<class Protocol, class DBAdapter>
	struct SHUTDOWN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_status(Status::SHUTDOWN);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"shutdown",	"SHUTDOWN"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "system";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				EXIT	,
				SHUTDOWN
			>(pack);
		}
	};



} // namespace


