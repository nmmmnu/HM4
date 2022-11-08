#include "base.h"



namespace net::worker::commands::Compat{

	namespace compat_impl_{

		template<class Protocol, class DBAdapter>
		struct OK : Base<Protocol,DBAdapter>{
			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				return result.set();
			}
		};

		template<class Protocol, class DBAdapter>
		struct STR : Base<Protocol,DBAdapter>{
			constexpr STR(std::string_view s) : s(s){
			}

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				return result.set(s);
			}

		private:
			std::string_view s;
		};

		template<class Protocol, class DBAdapter, typename T>
		struct SCALAR : Base<Protocol,DBAdapter>{
			constexpr SCALAR(T n) : n(n){
			}

			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				return result.set(n);
			}
		private:
			T n;
		};

	} // namespace compat_impl_


	template<class Protocol, class DBAdapter>
	struct SELECT : compat_impl_::OK<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "select";
		constexpr inline static std::string_view cmd[]	= {
			"select",	"SELECT"
		};
	};

	template<class Protocol, class DBAdapter>
	struct RESET : compat_impl_::OK<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "reset";
		constexpr inline static std::string_view cmd[]	= {
			"reset",	"RESET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct TYPE : compat_impl_::STR<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "type";
		constexpr inline static std::string_view cmd[]	= {
			"type",		"TYPE"
		};

		constexpr TYPE() : compat_impl_::STR<Protocol,DBAdapter>("string"){
		}
	};

	template<class Protocol, class DBAdapter>
	struct TOUCH : compat_impl_::SCALAR<Protocol,DBAdapter,bool>{
		constexpr inline static std::string_view name	= "touch";
		constexpr inline static std::string_view cmd[]	= {
			"touch",	"TOUCH"
		};

		constexpr TOUCH() : compat_impl_::SCALAR<Protocol,DBAdapter,bool>(true){
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
				TOUCH
			>(pack);
		}
	};



} // namespace


