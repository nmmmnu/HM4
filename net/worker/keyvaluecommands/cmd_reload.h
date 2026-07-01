#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct LISTMAINTAINANCE : BaseCommandRO<Protocol,DBAdapter>{
		LISTMAINTAINANCE() : BaseCommandRO<Protocol,DBAdapter>("LISTMAINTAINANCE", {
			"listmaintainance",		"LISTMAINTAINANCE"
		}){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db->crontab();

			return result.set();
		}

	};

	template<class Protocol, class DBAdapter>
	struct SAVE : BaseCommandRO<Protocol,DBAdapter>{
		SAVE() : BaseCommandRO<Protocol,DBAdapter>("SAVE", {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		}){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db.save();

			return result.set();
		}

	};

	template<class Protocol, class DBAdapter>
	struct RELOAD : BaseCommandRO<Protocol,DBAdapter>{
		RELOAD() : BaseCommandRO<Protocol,DBAdapter>("RELOAD", {
			"reload",	"RELOAD"
		}){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db.reload();

			return result.set();
		}

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "reload";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				LISTMAINTAINANCE	,
				SAVE			,
				RELOAD
			>(pack);
		}
	};



} // namespace


