#include "base.h"



namespace net::worker::commands::Compat{
	namespace compat_impl_{

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
			VAL(std::string_view name, CommandAliasesContainer const &cmd, T const &n) :
									BaseCommandRO<Protocol,DBAdapter>(name, cmd),
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

	} // namespace compat_impl_


	template<class Protocol, class DBAdapter>
	struct SELECT : compat_impl_::OK<Protocol,DBAdapter>{
		SELECT() : compat_impl_::OK<Protocol,DBAdapter>("SELECT",  {
				"select",	"SELECT"
		}){}
	};

	template<class Protocol, class DBAdapter>
	struct RESET : compat_impl_::OK<Protocol,DBAdapter>{
		RESET() : compat_impl_::OK<Protocol,DBAdapter>("RESET",  {
				"reset",	"RESET"
		}){}
	};

	template<class Protocol, class DBAdapter>
	struct TYPE : compat_impl_::VAL<Protocol,DBAdapter,std::string_view, compat_impl_::ResultType::SIMPLE_STRING>{
		constexpr TYPE() : compat_impl_::VAL<Protocol,DBAdapter,std::string_view, compat_impl_::ResultType::SIMPLE_STRING>(
			"TYPE", {
				"type",		"TYPE"
			},
			"string"
		){}
	};

	template<class Protocol, class DBAdapter>
	struct TOUCH : compat_impl_::VAL<Protocol,DBAdapter,bool>{
		constexpr TOUCH() : compat_impl_::VAL<Protocol,DBAdapter,bool>(
			"TOUCH", {
				"touch",	"TOUCH"
			},
			true
		){}
	};

	template<class Protocol, class DBAdapter>
	struct COMMAND : BaseCommandRO<Protocol,DBAdapter>{
		COMMAND() : BaseCommandRO<Protocol,DBAdapter>("COMMAND", {
				"command",	"COMMAND"
		}){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			// deliberatly send error
			return result.set_error(ResultErrorMessages::SYS_NOT_IMPLEMENTED);
		}

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


