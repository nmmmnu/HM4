#include "base.h"
#include "mytime.h"

namespace net::worker::commands::CoinBucket{
	namespace impl_{

		uint64_t calcTokens(uint64_t maxTokens, uint32_t maxSeconds, uint32_t created, uint32_t now, uint64_t tokens){
			auto const maxTokensF = static_cast<double>(maxTokens);
			auto const refillRate = maxTokensF / maxSeconds;

			auto const elapsed    = now >= created ? now - created : 0;

			auto const tokensNewF = std::min(
				maxTokensF,
				static_cast<double>(tokens) + elapsed * refillRate
			);

			return static_cast<uint64_t>(tokensNewF);
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct COINBUCKET : BaseCommandRW<Protocol,DBAdapter>{

		COINBUCKET() : BaseCommandRW<Protocol,DBAdapter>("COINBUCKET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// COINBUCKET key max_tokens max_seconds

		void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			uint64_t const maxTokens  = from_string<uint64_t>(p[2], 1);
			uint32_t const maxSeconds = from_string<uint32_t>(p[3], 1);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// Case 1: Old data exists

				auto const now = mytime::now32();

				auto const tokens = impl_::calcTokens(
							maxTokens			,
							maxSeconds			,
							mytime::to32(it->getCreated())	,
							now				,
							from_string<uint64_t>(it->getVal())
				);

				if (tokens == 0)
					return result.set_0();

				if (auto const expireAt = it->getExpiresAt(); now >= expireAt){
					// Corner case - we do not need to write at all:
					//
					// From first look, it looks like there will be "free" coins,
					// but this is not true.
					return result.set(tokens);
				}else{
					auto const tokens1 = tokens - 1;

					// this will not overflow
					to_string_buffer_t buffer;
					auto const val = to_string(tokens1, buffer);
					auto const exp = expireAt - now;

					// HINT
					const auto *hint = & *it;
					hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, key, val, exp);

					return result.set(tokens);
				}

			}else{
				// Case 2: Old data does not exists

				auto const tokens = maxTokens;

				auto const tokens1 = tokens - 1;

				to_string_buffer_t buffer;
				auto const val = to_string(tokens1, buffer);
				auto const exp = maxSeconds;

				hm4::insert(*db, key, val, exp);

				return result.set(tokens);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"coinbucket",		"COINBUCKET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct COINBUCKETGETCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		COINBUCKETGETCOUNT() : BaseCommandRO<Protocol,DBAdapter>("COINBUCKETGETCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// COINBUCKET key max_tokens max_seconds

		void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			uint64_t const maxTokens  = from_string<uint64_t>(p[2], 1);
			uint32_t const maxSeconds = from_string<uint32_t>(p[3], 1);

			if (auto *it = hm4::getPairPtr(*db, key); it){
				// Case 1: Old data exists

				auto const now = mytime::now32();

				auto tokens = impl_::calcTokens(
							maxTokens			,
							maxSeconds			,
							mytime::to32(it->getCreated())	,
							now				,
							from_string<uint64_t>(it->getVal())
				);

				return result.set(tokens);
			}else{
				return result.set(maxTokens);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"coinbucketgetcount",	"COINBUCKETGETCOUNT"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cb";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				COINBUCKET,
				COINBUCKETGETCOUNT
			>(pack);
		}
	};



} // namespace

