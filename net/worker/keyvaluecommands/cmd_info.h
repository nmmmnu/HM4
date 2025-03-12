#include "base.h"
#include "mytime.h"



namespace net::worker::commands::Info{



	template<class Protocol, class DBAdapter>
	struct INFO : BaseCmdRO<Protocol,DBAdapter>{
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
	struct DBSIZE : BaseCmdRO<Protocol,DBAdapter>{
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
	struct DBSIZEMUTABLE : BaseCmdRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return result.set(
				db->mutable_list().size()
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"dbsizemutable",	"DBSIZEMUTABLE"
		};
	};


	template<class Protocol, class DBAdapter>
	struct VERSION : BaseCmdRO<Protocol,DBAdapter>{
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
	struct MAXKEYSIZE : BaseCmdRO<Protocol,DBAdapter>{
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
	struct MAXVALSIZE : BaseCmdRO<Protocol,DBAdapter>{
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
	struct PING : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return result.set_simple_string("pong");
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ping",	"PING"
		};
	};



	template<class Protocol, class DBAdapter>
	struct ECHO : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &message = p[1];

			// message seamlessly, go to output buffer,
			// without interfering the input buffer.

			return result.set(message);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"echo",	"ECHO"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TIME : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			to_string_buffer_t buffer[2];

			auto const time = mytime::nowMix();

			return result.set_dual(
				to_string(time[0], buffer[0]),
				to_string(time[1], buffer[1])
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"time",	"TIME"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				INFO		,
				DBSIZE		,
				DBSIZEMUTABLE	,
				VERSION		,
				MAXKEYSIZE	,
				MAXVALSIZE	,
				PING		,
				ECHO		,
				TIME
			>(pack);
		}
	};



} // namespace


