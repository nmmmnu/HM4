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

		template<CPMVOperation operation, class Protocol, class DBAdapter>
		void cpmv(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			const auto &newkey = p[2];

			if (newkey.empty())
				return;

			if (key == newkey)
				return;

			if constexpr(operation == CPMVOperation::CP_NX || operation == CPMVOperation::MV_NX){
				if (impl_::exists(db, newkey))
					return result.set(false);
			}

			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				// SET

				db.set(newkey, it->getVal(), it->getTTL());

				if constexpr(operation == CPMVOperation::MV || operation == CPMVOperation::MV_NX){
					// DELETE

					db.del(key);
				}

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	}



	template<class Protocol, class DBAdapter>
	struct RENAME : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "rename";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"rename",	"RENAME"	,
			"move",		"MOVE"		,
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV>(p, db, result);
		}
	};



	template<class Protocol, class DBAdapter>
	struct RENAMENX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "renamenx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"renamenx",	"RENAMENX"	,
			"movenx",	"MOVENX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::MV_NX>(p, db, result);
		}
	};



	template<class Protocol, class DBAdapter>
	struct COPY : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "copy";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"copy",	"COPY"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP>(p, db, result);

		}
	};



	template<class Protocol, class DBAdapter>
	struct COPYNX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "copynx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"copynx",	"COPYNX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;
			return cpmv<CPMVOperation::CP_NX>(p, db, result);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				COPY		,
				COPYNX		,
				RENAME		,
				RENAMENX
			>(pack);
		}
	};


} // namespace


