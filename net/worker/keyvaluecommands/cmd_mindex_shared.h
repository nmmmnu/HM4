#include "base.h"

#include "shared_mset_multi.h"

namespace net::worker::commands::MIndexShared{

	template<class Protocol, class DBAdapter>
	struct IXMGET : BaseCommandRO<Protocol,DBAdapter>{
		IXMGET() : BaseCommandRO<Protocol,DBAdapter>("IXMGET", {
			"ixmget"	,	"IXMGET"	,
			"ixtget"	,	"IXTGET"	,
			"ixhget"	,	"IXHGET"
		}){}

		// IXMGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGet(p, db, result, blob);
		}

	};

	template<class Protocol, class DBAdapter>
	struct IXMMGET : BaseCommandRO<Protocol,DBAdapter>{
		IXMMGET() : BaseCommandRO<Protocol,DBAdapter>("IXMMGET", {
			"ixmmget"	,	"IXMMGET"	,
			"ixtmget"	,	"IXTMGET"	,
			"ixhmget"	,	"IXHMGET"
		}){}

		// IXMMGET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessMGet(p, db, result, blob);
		}

	};

	template<class Protocol, class DBAdapter>
	struct IXMGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		IXMGETINDEXES() : BaseCommandRO<Protocol,DBAdapter>("IXMGETINDEXES", {
			"ixmgetindexes"	,	"IXMGETINDEXES"	,
			"ixtgetindexes"	,	"IXTGETINDEXES"	,
			"ixhgetindexes"	,	"IXHGETINDEXES"
		}){}

		// IXMGETINDEXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessGetIndexes(p, db, result, blob);
		}

	};

	template<class Protocol, class DBAdapter>
	struct IXMREM : BaseCommandRW<Protocol,DBAdapter>{
		IXMREM() : BaseCommandRW<Protocol,DBAdapter>("IXMREM", {
			"ixmrem"	,	"IXMREM"	,
			"ixmremove"	,	"IXMREMOVE"	,
			"ixmdel"	,	"IXMDEL"	,

			"ixtrem"	,	"IXTREM"	,
			"ixtremove"	,	"IXTREMOVE"	,
			"ixtdel"	,	"IXTDEL"	,

			"ixhrem"	,	"IXHREM"	,
			"ixhremove"	,	"IXHREMOVE"	,
			"ixhdel"	,	"IXHDEL"
		}){}

		// IXMDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::msetmulti::cmdProcessRem(p, db, result, blob);
		}

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

