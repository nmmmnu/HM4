#include "base.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "info";
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			std::array<char, 1024> buffer;

			return result.set(
				db.info(buffer)
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct DBSIZE : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "dbsize";
		constexpr inline static std::string_view cmd[]	= {
			"dbsize",	"DBSIZE"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set(
				db.size()
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct VERSION : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "version";
		constexpr inline static std::string_view cmd[]	= {
			"version",	"VERSION"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set(
				db.version()
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct PING : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "ping";
		constexpr inline static std::string_view cmd[]	= {
			"ping",	"PING"
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set("pong");
		}
	};



	template<class Protocol, class DBAdapter>
	struct ECHO : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "echo";
		constexpr inline static std::string_view cmd[]	= {
			"echo",	"ECHO"
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			auto const&message = p[1];

			// message seamlessly, go to output buffer,
			// without interfering the input buffer.

			return result.set(message);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				INFO	,
				DBSIZE	,
				VERSION	,
				PING	,
				ECHO
			>(pack);
		}
	};



} // namespace


