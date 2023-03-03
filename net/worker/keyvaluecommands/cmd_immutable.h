#include "base.h"
#include "mystring.h"



namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			return result.set(
				hm4::getPairVal(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"get",	"GET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				container.emplace_back(
					hm4::getPairVal(*db, *itk)
				);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mget",	"MGET"
		};
	};


	template<class Protocol, class DBAdapter>
	struct EXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			return result.set(
				hm4::getPairOK(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"exists",	"EXISTS"
		};
	};


	template<class Protocol, class DBAdapter>
	struct TTL : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			uint64_t const ttl = hm4::getPair_(*db, key, [](bool b, auto it){
				return b ? it->getTTL() : 0;
			});

			return result.set(ttl);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ttl",	"TTL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETRANGE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return;

			const auto &key   = p[1];
			auto const start  = from_string<uint64_t>(p[2]);
			auto const finish = from_string<uint64_t>(p[3]);

			if (key.empty())
				return;

			if (finish < start)
				return result.set("");

			auto val = hm4::getPairVal(*db, key);

			if (start >= val.size())
				return result.set("");

			return result.set(
				val.substr(start, finish - start + 1)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getrange",	"GETRANGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct STRLEN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			return result.set(
				hm4::getPair_(*db, key, [](bool b, auto it) -> uint64_t{
					return b ? it->getVal().size() : 0;
				})
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"strlen",	"STRLEN"	,
			"size",		"SIZE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

			return result.set(
				hm4::getPairVal(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hget",	"HGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HMGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				container.emplace_back(
					hm4::getPairVal(*db, key)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hmget",	"HMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HEXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

			return result.set(
				hm4::getPairOK(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hexists",	"HEXISTS"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "immutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				GET		,
				MGET		,
				EXISTS		,
				TTL		,
				GETRANGE	,
				STRLEN		,
				HGET		,
				HMGET		,
				HEXISTS
			>(pack);
		}
	};



} // namespace

