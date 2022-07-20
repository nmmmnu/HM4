#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Immutable{



	template<class DBAdapter>
	struct GET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "get";
		constexpr inline static std::string_view cmd[]	= {
			"get",	"GET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			auto const &val = db.get(key);

			return Result::ok(val);
		}
	};



	template<class DBAdapter>
	struct EXISTS : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "exists";
		constexpr inline static std::string_view cmd[]	= {
			"exists",	"EXISTS"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			auto const &val = db.get(key);

			return Result::ok(!val.empty());
		}
	};



	template<class DBAdapter>
	struct TTL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "ttl";
		constexpr inline static std::string_view cmd[]	= {
			"ttl",	"TTL"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			auto const number = db.ttl(key);

			return Result::ok(number);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				GET	,
				EXISTS	,
				TTL
			>(pack);
		}
	};



} // namespace

