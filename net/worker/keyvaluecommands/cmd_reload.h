#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct LISTMAINTAINANCE : BaseCommandRO<Protocol,DBAdapter>{
		
		LISTMAINTAINANCE() : BaseCommandRO<Protocol,DBAdapter>("LISTMAINTAINANCE", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db->crontab();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"listmaintainance",		"LISTMAINTAINANCE"
		};

	};

	template<class Protocol, class DBAdapter>
	struct SAVE : BaseCommandRO<Protocol,DBAdapter>{
		
		SAVE() : BaseCommandRO<Protocol,DBAdapter>("SAVE", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db.save();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"save",		"SAVE",
			"bgsave",	"BGSAVE"
		};

	};

	template<class Protocol, class DBAdapter>
	struct RELOAD : BaseCommandRO<Protocol,DBAdapter>{
		
		RELOAD() : BaseCommandRO<Protocol,DBAdapter>("RELOAD", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db.reload();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"reload",	"RELOAD"
		};

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


