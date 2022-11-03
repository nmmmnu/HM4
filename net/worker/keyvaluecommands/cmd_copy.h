#include "base.h"



namespace net::worker::commands::Copy{

	namespace impl_{
		template<class DBAdapter>
		auto exists(DBAdapter &db, std::string_view key){
			auto it = db.find(key);

			return it && it->isValid(std::true_type{});
		}

		enum class CPMVOperation{
			MV	,
			MV_NX	,
			CP	,
			CP_NX
		};

		template<CPMVOperation operation, class DBAdapter>
		Result cpmv(ParamContainer const &p, DBAdapter &db){
			if (p.size() != 3)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			const auto &newkey = p[2];

			if (newkey.empty())
				return Result::error();

			if (key == newkey)
				return Result::error();

			if constexpr(operation == CPMVOperation::CP_NX || operation == CPMVOperation::MV_NX){
				if (impl_::exists(db, newkey))
					return Result::ok(false);
			}

			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				// SET

				db.set(newkey, it->getVal(), it->getTTL());

				if constexpr(operation == CPMVOperation::MV || operation == CPMVOperation::MV_NX){
					// DELETE

					db.del(key);
				}

				return Result::ok(true);
			}else{
				return Result::ok(false);
			}
		}

	}



	template<class DBAdapter>
	struct RENAME : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "rename";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"rename",	"RENAME"	,
			"move",		"MOVE"		,
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV>(p, db);
		}
	};



	template<class DBAdapter>
	struct RENAMENX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "renamenx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"renamenx",	"RENAMENX"	,
			"movenx",	"MOVENX"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV_NX>(p, db);
		}
	};



	template<class DBAdapter>
	struct COPY : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "copy";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"copy",	"COPY"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP>(p, db);

		}
	};



	template<class DBAdapter>
	struct COPYNX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "copynx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"copynx",	"COPYNX"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP_NX>(p, db);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				COPY		,
				COPYNX		,
				RENAME		,
				RENAMENX
			>(pack);
		}
	};


} // namespace


