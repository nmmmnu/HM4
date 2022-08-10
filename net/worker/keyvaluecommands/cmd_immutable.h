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

			auto it = db.find(key);

			auto const val = it && it->isValid(std::true_type{}) ? it->getVal() : "";

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

			auto it = db.find(key);

			return Result::ok(
				it && it->isValid(std::true_type{})
			);
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

			auto it = db.find(key);

			auto ttl = it && it->isValid(std::true_type{}) ? it->getTTL() : 0;

			return Result::ok(
				static_cast<uint64_t>(ttl)
			);
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

