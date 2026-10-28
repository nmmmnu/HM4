#include "base.h"
#include "mytime.h"
#include "listchain.h"


namespace net::worker::commands::Info{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct INFO : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		INFO() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("INFO", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			std::array<char, 2048> buffer;

			return result.set(
				db. template info<MemoryManager>(buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"info",	"INFO"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct DBSIZE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		DBSIZE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("DBSIZE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(
				db->size()
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"dbsize",	"DBSIZE"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct DBSIZEMUTABLE : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		DBSIZEMUTABLE() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("DBSIZEMUTABLE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(
				db->chain(hm4::chain::mutable_list{})->size()
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"dbsizemutable",	"DBSIZEMUTABLE"
		};

	};


	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct VERSION : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		VERSION() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("VERSION", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(
				db.version()
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"version",	"VERSION"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct MAXKEYSIZE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		MAXKEYSIZE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("MAXKEYSIZE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(uint64_t{ hm4::PairConf::MAX_KEY_SIZE });
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"maxkeysize",	"MAXKEYSIZE"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct MAXVALSIZE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		MAXVALSIZE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("MAXVALSIZE", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set(uint64_t{ hm4::PairConf::MAX_VAL_SIZE });
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"maxvalsize",	"MAXVALSIZE"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PING : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PING() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PING", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set_simple_string("pong");
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ping",	"PING"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct ECHO : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		ECHO() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("ECHO", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &message = p[1];

			// message seamlessly, go to output buffer,
			// without interfering the input buffer.

			return result.set(message);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"echo",	"ECHO"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct REM : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		REM() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("REM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"rem",	"REM",
			"//",	"#",	";"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct TIME : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		TIME() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("TIME", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			to_string_buffer_t buffer[2];

			auto const time = mytime::nowMix();

			return result.set_containerN(
				to_string(time[0], buffer[0]),
				to_string(time[1], buffer[1])
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"time",	"TIME"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "info";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				INFO		,
				DBSIZE		,
				DBSIZEMUTABLE	,
				VERSION		,
				MAXKEYSIZE	,
				MAXVALSIZE	,
				PING		,
				ECHO		,
				REM		,
				TIME
			>(pack);
		}
	};



} // namespace


