#include "base.h"



namespace net::worker::commands::Compat{

	namespace compat_impl_{

		template<class Protocol, class DBAdapter>
		struct OK : Base<Protocol,DBAdapter>{
			constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
				return result.set();
			}
		};

		template<class Protocol, class DBAdapter, typename T>
		struct VAL : Base<Protocol,DBAdapter>{
			constexpr VAL(T const &n) : n(n){
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
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"select",	"SELECT"
		};
	};

	template<class Protocol, class DBAdapter>
	struct RESET : compat_impl_::OK<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"reset",	"RESET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct TYPE : compat_impl_::VAL<Protocol,DBAdapter,std::string_view>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr TYPE() : compat_impl_::VAL<Protocol,DBAdapter,std::string_view>("string"){
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"type",		"TYPE"
		};
	};

	template<class Protocol, class DBAdapter>
	struct TOUCH : compat_impl_::VAL<Protocol,DBAdapter,bool>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr TOUCH() : compat_impl_::VAL<Protocol,DBAdapter,bool>(true){
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"touch",	"TOUCH"
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
				TOUCH
			>(pack);
		}
	};



} // namespace


