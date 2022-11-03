#include "base.h"



namespace net::worker::commands::CAS{



	template<class DBAdapter>
	struct CAS : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "cas";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"cas",	"CAS"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 4 && p.size() != 5)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			const auto &old_val = p[2];

			if (old_val.empty())
				return Result::error();

			const auto &val = p[3];

			if (val.empty())
				return Result::error();

			if (auto it = db.find(key);
					it && it->isValid(std::true_type{}) &&
					it->getVal() == old_val){
				// SET

				auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

				db.setHint(& *it, val, exp);

				return Result::ok(true);
			}

			return Result::ok(false);
		}
	};



	template<class DBAdapter>
	struct CAD : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "cad";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"cad",	"CAD"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 3)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			const auto &old_val = p[2];

			if (old_val.empty())
				return Result::error();

			if (auto it = db.find(key);
					it && it->isValid(std::true_type{}) &&
					it->getVal() == old_val){
				// DEL

				db.delHint(& *it);

				return Result::ok(true);
			}

			return Result::ok(false);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cas";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				CAS		,
				CAD
			>(pack);
		}
	};


} // namespace


