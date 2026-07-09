#include "base.h"



namespace net::worker::commands::Compat{
	namespace impl_{

		template<class Protocol, class DBAdapter>
		struct OK : BaseCommandRO<Protocol,DBAdapter>{
			using BaseCommandRO<Protocol,DBAdapter>::BaseCommandRO;

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				return result.set();
			}
		};

		enum class ResultType{
			NORMAL		,
			SIMPLE_STRING
		};

		template<class Protocol, class DBAdapter, typename T, ResultType RT = ResultType::NORMAL>
		struct VAL : BaseCommandRO<Protocol,DBAdapter>{
			VAL(std::string_view name, const std::string_view *cmd_begin, const std::string_view *cmd_end, T const &n) :
									BaseCommandRO<Protocol,DBAdapter>(name, cmd_begin, cmd_end),
									n(n){}

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				if constexpr(RT == ResultType::SIMPLE_STRING)
					return result.set_simple_string(n);
				else
					return result.set(n);
			}

		private:
			T n;
		};

	} // namespace impl_


	template<class Protocol, class DBAdapter>
	struct SELECT : impl_::OK<Protocol,DBAdapter>{
		SELECT() : impl_::OK<Protocol,DBAdapter>("SELECT", std::begin(cmd__), std::end(cmd__)){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"select",	"SELECT"
		};
	};

	template<class Protocol, class DBAdapter>
	struct RESET : impl_::OK<Protocol,DBAdapter>{
		RESET() : impl_::OK<Protocol,DBAdapter>("RESET", std::begin(cmd__), std::end(cmd__)){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"reset",		"RESET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct TYPE : impl_::VAL<Protocol,DBAdapter,std::string_view, impl_::ResultType::SIMPLE_STRING>{
		constexpr TYPE() : impl_::VAL<Protocol,DBAdapter,std::string_view, impl_::ResultType::SIMPLE_STRING>(
			"TYPE", std::begin(cmd__), std::end(cmd__), "string"){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"type",		"TYPE"
		};
	};

	template<class Protocol, class DBAdapter>
	struct TOUCH : impl_::VAL<Protocol,DBAdapter,bool>{
		constexpr TOUCH() : impl_::VAL<Protocol,DBAdapter,bool>("TOUCH", std::begin(cmd__), std::end(cmd__), true){}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"touch",	"TOUCH"
		};
	};

	template<class Protocol, class DBAdapter>
	struct COMMAND : BaseCommandRO<Protocol,DBAdapter>{

		COMMAND() : BaseCommandRO<Protocol,DBAdapter>("COMMAND", std::begin(cmd__), std::end(cmd__)){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			// deliberatly send error
			return result.set_error(ResultErrorMessages::SYS_NOT_IMPLEMENTED);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
				"command",	"COMMAND"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "compat";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SELECT	,
				RESET	,
				TYPE	,
				TOUCH	,
				COMMAND
			>(pack);
		}
	};



} // namespace


