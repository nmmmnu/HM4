#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "get";
		constexpr inline static std::string_view cmd[]	= {
			"get",	"GET"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto it = db.find(key);

			auto const val = it && it->isValid(std::true_type{}) ? it->getVal() : "";

			return result.set(val);
		}
	};



	template<class Protocol, class DBAdapter>
	struct MGET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "mget";
		constexpr inline static std::string_view cmd[]	= {
			"mget",	"MGET"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 2)
				return;

			auto &container = blob.container;

			auto const varg = 1;

			if (container.capacity() < p.size() - varg)
				return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			container.clear();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;

				auto it = db.find(key);

				container.emplace_back(
					it && it->isValid(std::true_type{}) ? it->getVal() : ""
				);
			}

			return result.set_container(container);
		}
	};



	template<class Protocol, class DBAdapter>
	struct EXISTS : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "exists";
		constexpr inline static std::string_view cmd[]	= {
			"exists",	"EXISTS"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto it = db.find(key);

			return result.set(
				it && it->isValid(std::true_type{})
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct TTL : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "ttl";
		constexpr inline static std::string_view cmd[]	= {
			"ttl",	"TTL"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto it = db.find(key);

			auto ttl = it && it->isValid(std::true_type{}) ? it->getTTL() : 0;

			return result.set(
				uint64_t{ttl}
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct STRLEN : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "strlen";
		constexpr inline static std::string_view cmd[]	= {
			"strlen",	"STRLEN"	,
			"size",		"SIZE"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto it = db.find(key);

			auto size = it && it->isValid(std::true_type{}) ? it->getVal().size() : 0;

			return result.set(
				uint64_t{size}
			);
		}
	};



	template<class Protocol, class DBAdapter>
	struct HGET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hget";
		constexpr inline static std::string_view cmd[]	= {
			"hget",	"HGET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			const auto &subN = p[2];

			if (subN.empty())
				return;

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

			auto it = db.find(key);

			auto const val = it && it->isValid(std::true_type{}) ? it->getVal() : "";

			return result.set(val);
		}
	};



	template<class Protocol, class DBAdapter>
	struct HMGET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hmget";
		constexpr inline static std::string_view cmd[]	= {
			"hmget",	"HMGET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			auto &container = blob.container;

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return;

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return;
			}

			container.clear();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				auto it = db.find(key);

				container.emplace_back(
					it && it->isValid(std::true_type{}) ? it->getVal() : ""
				);
			}

			return result.set_container(container);
		}
	};



	template<class Protocol, class DBAdapter>
	struct HEXISTS : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hexists";
		constexpr inline static std::string_view cmd[]	= {
			"hexists",	"HEXISTS"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			const auto &subN = p[2];

			if (subN.empty())
				return;

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

			auto it = db.find(key);

			return result.set(
				it && it->isValid(std::true_type{})
			);
		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
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

