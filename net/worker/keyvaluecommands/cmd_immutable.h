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



	template<class DBAdapter>
	struct HGET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hget";
		constexpr inline static std::string_view cmd[]	= {
			"hget",	"HGET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			const auto &subN = p[2];

			if (subN.empty())
				return Result::error();

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const key = concatenateBuffer(blob.string_key, keyN, DBAdapter::SEPARATOR, subN);

			auto it = db.find(key);

			auto const val = it && it->isValid(std::true_type{}) ? it->getVal() : "";

			return Result::ok(val);
		}
	};



	template<class DBAdapter>
	struct HEXISTS : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hexists";
		constexpr inline static std::string_view cmd[]	= {
			"hexists",	"HEXISTS"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			const auto &subN = p[2];

			if (subN.empty())
				return Result::error();

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const key = concatenateBuffer(blob.string_key, keyN, DBAdapter::SEPARATOR, subN);

			auto it = db.find(key);

			return Result::ok(
				it && it->isValid(std::true_type{})
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
				TTL	,
				HGET	,
				HEXISTS
			>(pack);
		}
	};



} // namespace

