#include "base.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			std::array<char, 1024> buffer;

			return result.set(
				db.info(buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"info",	"INFO"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DBSIZE : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set(
				db->size()
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"dbsize",	"DBSIZE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct VERSION : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set(
				db.version()
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"version",	"VERSION"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PING : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			return result.set("pong");
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ping",	"PING"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ECHO : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			auto const&message = p[1];

			// message seamlessly, go to output buffer,
			// without interfering the input buffer.

			return result.set(message);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"echo",	"ECHO"
		};
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


