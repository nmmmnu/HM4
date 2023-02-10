#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct SAVE : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			db.save();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		};
	};

	template<class Protocol, class DBAdapter>
	struct RELOAD : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return;

			db.reload();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"reload",	"RELOAD"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "reload";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SAVE	,
				RELOAD
			>(pack);
		}
	};



} // namespace


