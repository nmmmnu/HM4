#include "base.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			std::array<char, 2048> buffer;

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
	struct DBSIZE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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
	struct VERSION : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
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
	struct MAXKEYSIZE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set(uint64_t{ hm4::PairConf::MAX_KEY_SIZE });
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"maxkeysize",	"MAXKEYSIZE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MAXVALSIZE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set(uint64_t{ hm4::PairConf::MAX_VAL_SIZE });
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"maxvalsize",	"MAXVALSIZE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PING : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set("pong");
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ping",	"PING"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ECHO : BaseRO<Protocol,DBAdapter>{
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
				INFO		,
				DBSIZE		,
				VERSION		,
				MAXKEYSIZE	,
				MAXVALSIZE	,
				PING		,
				ECHO
			>(pack);
		}
	};



} // namespace


