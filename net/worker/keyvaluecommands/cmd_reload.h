#include "base.h"



namespace net::worker::commands::Reload{



	template<class Protocol, class DBAdapter>
	struct LISTMAINTAINANCE : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

			db->crontab();

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"listmaintainance",		"LISTMAINTAINANCE"
		};
	};

	template<class Protocol, class DBAdapter>
	struct SAVE : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

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
	struct RELOAD : BaseCmdRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_0);

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
				LISTMAINTAINANCE	,
				SAVE			,
				RELOAD
			>(pack);
		}
	};



} // namespace


