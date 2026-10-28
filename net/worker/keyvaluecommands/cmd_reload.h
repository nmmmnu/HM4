#include "base.h"



namespace net::worker::commands::Reload{



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct LISTMAINTAINANCE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		LISTMAINTAINANCE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("LISTMAINTAINANCE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
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

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct SAVE : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		SAVE() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("SAVE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
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

	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct RELOAD : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		RELOAD() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("RELOAD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
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



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "reload";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				LISTMAINTAINANCE	,
				SAVE			,
				RELOAD
			>(pack);
		}
	};



} // namespace


