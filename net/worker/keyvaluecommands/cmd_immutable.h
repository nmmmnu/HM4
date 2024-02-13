#include "base.h"
#include "mystring.h"

#include "shared_hash.h"

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto &container = blob.container();

			auto const varg = 1;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
	struct EXPIRETIME : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			uint64_t const time = hm4::getPair_(*db, key, [](bool b, auto it){
				return b ? it->getExpiresAt() : 0;
			});

			return result.set(time);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"expireat",	"EXPIRETIME"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DUMP : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const *pair = hm4::getPairPtrNC(*db, key);

			if (pair){
				std::string_view const x{
					reinterpret_cast<const char *>(pair),
					pair->bytes()
				};

				return result.set(x);
			}else{
				return result.set("");
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"dump",	"DUMP"
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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &key   = p[1];
			auto const start  = from_string<uint64_t>(p[2]);
			auto const finish = from_string<uint64_t>(p[3]);

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &subN = p[2];

			if (subN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::isHKeyValid(keyN, subN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.container();

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!hm4::isHKeyValid(keyN, subN))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				hm4::PairBufferKey bufferKey;
				auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

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

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &subN = p[2];

			if (subN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!hm4::isHKeyValid(keyN, subN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

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
				EXPIRETIME	,
				DUMP		,
				GETRANGE	,
				STRLEN		,
				HGET		,
				HMGET		,
				HEXISTS
			>(pack);
		}
	};



} // namespace

