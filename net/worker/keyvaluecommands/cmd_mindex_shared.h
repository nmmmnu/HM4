#include "base.h"

#include "shared_mset_multi.h"

namespace net::worker::commands::MIndexShared{

	template<class Protocol, class DBAdapter>
	struct IXMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGet(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmget",	"IXMGET"	,
			"ixtget",	"IXTGET"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMMGET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessMGet(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmmget",	"IXMMGET",
			"ixtmget",	"IXTMGET"
		};


	};

	template<class Protocol, class DBAdapter>
	struct IXMGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMGETINDEXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGetIndexes(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmgetindexes",	"IXMGETINDEXES",
			"ixtgetindexes",	"IXTGETINDEXES"
		};
	};

	template<class Protocol, class DBAdapter>
	struct IXMREM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// IXMDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessRem(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ixmrem"	,	"IXMREM"	,
			"ixmremove"	,	"IXMREMOVE"	,
			"ixmdel"	,	"IXMDEL"	,

			"ixtrem"	,	"IXTREM"	,
			"ixtremove"	,	"IXTREMOVE"	,
			"ixtdel"	,	"IXTDEL"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex_shared";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMGET		,
				IXMMGET		,
				IXMGETINDEXES	,
				IXMREM
			>(pack);
		}
	};

} // namespace net::worker::commands::MIndexShared

