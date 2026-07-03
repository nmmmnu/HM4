#include "base.h"
#include "mystring.h"

namespace net::worker::commands::Immutable{



	template<class Protocol, class DBAdapter>
	struct GET : BaseCommandRO<Protocol,DBAdapter>{
		
		GET() : BaseCommandRO<Protocol,DBAdapter>("GET", std::begin(cmd__), std::end(cmd__)){}


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
		constexpr inline static std::string_view cmd__[] = {
			"get",	"GET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct MGET : BaseCommandRO<Protocol,DBAdapter>{
		
		MGET() : BaseCommandRO<Protocol,DBAdapter>("MGET", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				container.emplace_back(
					hm4::getPairVal(*db, *itk)
				);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"mget",	"MGET"
		};

	};


	template<class Protocol, class DBAdapter>
	struct EXISTS : BaseCommandRO<Protocol,DBAdapter>{
		
		EXISTS() : BaseCommandRO<Protocol,DBAdapter>("EXISTS", std::begin(cmd__), std::end(cmd__)){}


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
		constexpr inline static std::string_view cmd__[] = {
			"exists",	"EXISTS"
		};

	};



	template<class Protocol, class DBAdapter>
	struct TTL : BaseCommandRO<Protocol,DBAdapter>{
		
		TTL() : BaseCommandRO<Protocol,DBAdapter>("TTL", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			uint64_t const ttl = hm4::getPairOK_(*db, key, [](bool b, const auto *p){
				return b ? p->getTTL() : 0;
			});

			return result.set(ttl);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ttl",	"TTL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct EXPIRETIME : BaseCommandRO<Protocol,DBAdapter>{
		
		EXPIRETIME() : BaseCommandRO<Protocol,DBAdapter>("EXPIRETIME", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			uint64_t const time = hm4::getPairOK_(*db, key, [](bool b, const auto *p){
				return b ? p->getExpiresAt() : 0;
			});

			return result.set(time);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"expiretime",	"EXPIRETIME"
		};

	};



	template<class Protocol, class DBAdapter>
	struct DUMP : BaseCommandRO<Protocol,DBAdapter>{
		
		DUMP() : BaseCommandRO<Protocol,DBAdapter>("DUMP", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (const auto *p = hm4::getPair(*db, key); p){
				std::string_view const x{
					reinterpret_cast<const char *>(p),
					p->bytes()
				};

				return result.set(x);
			}else{
				return result.set("");
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"dump",	"DUMP"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GETRANGE : BaseCommandRO<Protocol,DBAdapter>{
		
		GETRANGE() : BaseCommandRO<Protocol,DBAdapter>("GETRANGE", std::begin(cmd__), std::end(cmd__)){}


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
		constexpr inline static std::string_view cmd__[] = {
			"getrange",	"GETRANGE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct STRLEN : BaseCommandRO<Protocol,DBAdapter>{
		
		STRLEN() : BaseCommandRO<Protocol,DBAdapter>("STRLEN", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			return result.set(
				hm4::getPairOK_(*db, key, [](bool b, const auto *p) -> uint64_t{
					return b ? p->getValLen() : 0;
				})
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"strlen",	"STRLEN"	,
			"size",		"SIZE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HGET : BaseCommandRO<Protocol,DBAdapter>{
		
		HGET() : BaseCommandRO<Protocol,DBAdapter>("HGET", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];
			auto const &subN = p[2];

			if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

			return result.set(
				hm4::getPairVal(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"hget",	"HGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HMGET : BaseCommandRO<Protocol,DBAdapter>{
		
		HMGET() : BaseCommandRO<Protocol,DBAdapter>("HMGET", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &container = blob.construct<OutputBlob::Container>();

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
		constexpr inline static std::string_view cmd__[] = {
			"hmget",	"HMGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct HEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		
		HEXISTS() : BaseCommandRO<Protocol,DBAdapter>("HEXISTS", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &keyN = p[1];
			const auto &subN = p[2];

			if (!hm4::Pair::isCompositeKeyValid(1, keyN, subN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, subN);

			return result.set(
				hm4::getPairOK(*db, key)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
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

