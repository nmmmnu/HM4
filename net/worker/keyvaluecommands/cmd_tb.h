#include "base.h"
#include "mytime.h"

namespace net::worker::commands::TokenBucket{
	namespace impl_{

		constexpr uint64_t calcTokens(uint64_t maxTokens, uint32_t maxSeconds, uint32_t created, uint32_t now, uint64_t tokens){
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
	struct TBCONSUME : BaseCommandRW<Protocol,DBAdapter>{

		TBCONSUME() : BaseCommandRW<Protocol,DBAdapter>("TBCONSUME", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// TBCONSUME key max_tokens max_seconds price=1

		void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4 && p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto     const maxTokensSV	= p[2];
			uint64_t const maxTokens	= from_string<uint64_t>(p[2], 1);
			uint32_t const maxSeconds	= from_string<uint32_t>(p[3], 1);
			uint16_t const price		= p.size() == 5 ? from_string<uint16_t>(p[4], 1) : 1u;

			if (!price || price > maxTokens)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

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

				if (tokens < price){
					to_string_buffer_t buffer;
					return result.set_containerN("0", to_string(tokens, buffer));
				}

				if (auto const expireAt = it->getExpiresAt(); now >= expireAt){
					// Corner case - we do not need to write at all:
					//
					// From first look, it looks like there will be "free" coins,
					// but this is not true.
					//
					// When hm4::getPairPtr() was executed, the pair was NOT expired,
					// also it had enought coins to pass.
					//
					// However at the time of now >= expireAt, the pair expired.
					//
					// This means:
					// 1. the coinbucked is fully refill.
					// 2. we do not need to update or delete.

					return result.set_containerN("1", maxTokensSV);
				}else{
					auto const tokens1 = tokens - price;

					// this will not overflow
					to_string_buffer_t buffer;
					auto const val = to_string(tokens1, buffer);
					auto const exp = expireAt - now;

					// HINT
					const auto *hint = & *it;
					hm4::insertHintF<hm4::PairFactory::Normal>(*db, hint, key, val, exp);

					return result.set_containerN("1", val);
				}

			}else{
				// Case 2: Old data does not exists

				auto const tokens = maxTokens;

				auto const tokens1 = tokens - price;

				to_string_buffer_t buffer;
				auto const val = to_string(tokens1, buffer);
				auto const exp = maxSeconds;

				hm4::insert(*db, key, val, exp);

				return result.set_containerN("1", val);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"tbconsume",		"TBCONSUME"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TBCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		TBCOUNT() : BaseCommandRO<Protocol,DBAdapter>("TBCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// TBCOUNT key max_tokens max_seconds

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
			"tbcount",	"TBCOUNT"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cb";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				TBCONSUME,
				TBCOUNT
			>(pack);
		}
	};



} // namespace

