#include "base.h"



namespace net::worker::commands::CAS{



	template<class Protocol, class DBAdapter>
	struct CAS : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "cas";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"cas",	"CAS"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4 && p.size() != 5)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			const auto &old_val = p[2];

			if (old_val.empty())
				return;

			const auto &val = p[3];

			if (val.empty())
				return;

			if (auto it = db.find(key);
					it && it->isValid(std::true_type{}) &&
					it->getVal() == old_val){
				// SET

				auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

				db.setHint(& *it, val, exp);

				return result.set(true);
			}

			return result.set(false);
		}
	};



	template<class Protocol, class DBAdapter>
	struct CAD : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "cad";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"cad",	"CAD"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			const auto &old_val = p[2];

			if (old_val.empty())
				return;

			if (auto it = db.find(key);
					it && it->isValid(std::true_type{}) &&
					it->getVal() == old_val){
				// DEL

				db.delHint(& *it);

				return result.set(true);
			}

			return result.set(false);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cas";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CAS		,
				CAD
			>(pack);
		}
	};


} // namespace


