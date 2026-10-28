#include "base.h"



namespace net::worker::commands::Compat{
	namespace impl_{

		template<typename Protocol, typename MemoryManager, typename DBAdapter>
		struct OK : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{
			using BaseCommandRO<Protocol,MemoryManager,DBAdapter>::BaseCommandRO;

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
				return result.set();
			}
		};

		enum class ResultType{
			NORMAL		,
			SIMPLE_STRING
		};

		template<class Protocol, class MemoryManager, class DBAdapter, typename T, ResultType RT = ResultType::NORMAL>
		struct VAL : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{
			VAL(std::string_view name, const std::string_view *cmd_begin, const std::string_view *cmd_end, T const &n) :
									BaseCommandRO<Protocol,MemoryManager,DBAdapter>(name, cmd_begin, cmd_end),
									n(n){}

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
				if constexpr(RT == ResultType::SIMPLE_STRING)
					return result.set_simple_string(n);
				else
					return result.set(n);
			}

		private:
			T n;
		};

	} // namespace impl_


	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct SELECT : impl_::OK<Protocol,MemoryManager,DBAdapter>{
		SELECT() : impl_::OK<Protocol,MemoryManager,DBAdapter>("SELECT", std::begin(cmd__), std::end(cmd__)){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"select",	"SELECT"
		};
	};

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct RESET : impl_::OK<Protocol,MemoryManager,DBAdapter>{
		RESET() : impl_::OK<Protocol,MemoryManager,DBAdapter>("RESET", std::begin(cmd__), std::end(cmd__)){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"reset",		"RESET"
		};
	};

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct TYPE : impl_::VAL<Protocol, MemoryManager, DBAdapter, std::string_view, impl_::ResultType::SIMPLE_STRING>{
		constexpr TYPE() : impl_::VAL<Protocol, MemoryManager, DBAdapter, std::string_view, impl_::ResultType::SIMPLE_STRING>(
			"TYPE", std::begin(cmd__), std::end(cmd__), "string"){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"type",		"TYPE"
		};
	};

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct TOUCH : impl_::VAL<Protocol,MemoryManager,DBAdapter,bool>{
		constexpr TOUCH() : impl_::VAL<Protocol,MemoryManager,DBAdapter,bool>("TOUCH", std::begin(cmd__), std::end(cmd__), true){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"touch",	"TOUCH"
		};
	};

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct COMMAND : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		COMMAND() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("COMMAND", std::begin(cmd__), std::end(cmd__)){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			// deliberatly send error
			return result.set_error(ResultErrorMessages::SYS_NOT_IMPLEMENTED);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
				"command",	"COMMAND"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "compat";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				SELECT	,
				RESET	,
				TYPE	,
				TOUCH	,
				COMMAND
			>(pack);
		}
	};



} // namespace


