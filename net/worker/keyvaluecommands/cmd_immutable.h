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
	struct MGET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "mget";
		constexpr inline static std::string_view cmd[]	= {
			"mget",	"MGET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() < 2)
				return Result::error();

			auto &container = blob.container;

			auto const varg = 1;

			if (container.capacity() < p.size() - varg)
				return Result::error();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			container.clear();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;

				auto it = db.find(key);

				container.emplace_back(
					it && it->isValid(std::true_type{}) ? it->getVal() : ""
				);
			}

			return Result::ok_container(container);
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
				uint64_t{ttl}
			);
		}
	};



	template<class DBAdapter>
	struct STRLEN : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "strlen";
		constexpr inline static std::string_view cmd[]	= {
			"strlen",	"STRLEN"	,
			"size",		"SIZE"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			auto it = db.find(key);

			auto size = it && it->isValid(std::true_type{}) ? it->getVal().size() : 0;

			return Result::ok(
				uint64_t{size}
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
	struct HMGET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hmget";
		constexpr inline static std::string_view cmd[]	= {
			"hmget",	"HMGET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() < 3)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			auto &container = blob.container;

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return Result::error();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return Result::error();

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return Result::error();
			}

			container.clear();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				auto const key = concatenateBuffer(blob.string_key, keyN, DBAdapter::SEPARATOR, subN);

				auto it = db.find(key);

				container.emplace_back(
					it && it->isValid(std::true_type{}) ? it->getVal() : ""
				);
			}

			return Result::ok_container(container);
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
				MGET	,
				EXISTS	,
				TTL	,
				STRLEN	,
				HGET	,
				HMGET	,
				HEXISTS
			>(pack);
		}
	};



} // namespace

