#include "base.h"
#include "idgenerator.h"
#include "mystring.h"
#include "logger.h"
#include "ilist/txguard.h"


namespace net::worker::commands::Queue{
	namespace queue_impl_{
		using namespace net::worker::shared::config;

		using MyIDGenerator = idgenerator::IDGeneratorTS_HEX;

		constexpr bool isKeyValid(std::string_view keyN){
			return hm4::Pair::isCompositeKeyValid(MyIDGenerator::to_string_buffer_t_size + 1, keyN);
		}

	} // namespace



	template<class Protocol, class DBAdapter>
	struct SADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyIDGenerator = queue_impl_::MyIDGenerator;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace queue_impl_;

			if (p.size() != 3 && p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_23);

			auto const &keyN = p[1];

			if (!isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const &val = p[2];

			if (!hm4::Pair::isValValid(val))
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			MyIDGenerator::to_string_buffer_t buffer;
			auto const id = MyIDGenerator{}(buffer);

			hm4::PairBufferKey bufferKey;
			auto const key = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, id);

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sadd",		"SADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SPOP : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyIDGenerator = queue_impl_::MyIDGenerator;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace queue_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			// GET
			const auto &keyN = p[1];

			if (!isKeyValid(keyN))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;
			auto const keyControl = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR);

			auto it = db->find(keyControl);

			if (it == std::end(*db)){
				// case 1. it == end, no any data

				return result.set("");
			}

			if (it->getKey() == keyControl){
				// case 2. there is a control key

				if (it->isOK()){
					// case 2.1. control key is valid

					logger<Logger::DEBUG>() << "SPOP: Control key" << keyControl << "is valid";

					if (auto const keySubNext = it->getVal(); keySubNext.size() <= MyIDGenerator::to_string_buffer_t_size){
						// case 2.1.1 score seems legit, go to case 3

						hm4::PairBufferKey bufferKey;
						auto const keyNext = concatenateBuffer(bufferKey, keyN, DBAdapter::SEPARATOR, it->getVal());

						logger<Logger::DEBUG>() << "SPOP: next key" << keyNext;

						return collect_(
							keyControl		,
							db->find(keyNext)	,
							std::end(*db)		,
							*db,
							result
						);
					}
				}

				// case 2.2. control key is NOT valid, or score is invalid

				logger<Logger::DEBUG>() << "SPOP: Control key" << keyControl << "is NOT valid or contains invalid data";

				++it;

				if (it == std::end(*db)){
					// case 2.2.1. it == end, no any data

					return result.set("");
				}

				// go to case 3
			}

			// case 3, search next keys...

			logger<Logger::DEBUG>() << "SPOP: Next key for control key" << it->getKey();

			return collect_(
				keyControl,
				it,
				std::end(*db),
				*db,
				result
			);
		}

	private:
		static void collect_(std::string_view keyControl, typename DBAdapter::List::iterator it, typename DBAdapter::List::iterator eit, typename DBAdapter::List &list, Result<Protocol> &result){
			using namespace queue_impl_;

			uint32_t iterations = 0;

			for(;it != eit;++it){
				auto const &key = it->getKey();

				if (! same_prefix(keyControl, key)){
					logger<Logger::DEBUG>() << "SPOP: New prefix, done";
					return finalizeEnd_(keyControl, list, result);
				}

				if (it->isOK()){
					logger<Logger::DEBUG>() << "SPOP: Valid key, done";
					auto const &val = it->getVal();
					return finalizeOK_(keyControl, key, val, list, result, iterations);
				}

				if (++iterations > ITERATIONS_LOOPS_MAX){
					logger<Logger::DEBUG>() << "SPOP: Lots of iterations, done";
					return finalizeTryAgain_(keyControl, key, list, result);
				}
			}

			// it == end
			return finalizeEnd_(keyControl, list, result);
		}

		constexpr auto static getScore_(std::size_t keyControl_size, std::string_view key){
			return key.substr(keyControl_size);
		}

		constexpr auto static getScore_(std::string_view keyControl, std::string_view key){
			return getScore_(keyControl.size(), key);
		}

		static void finalizeOK_(std::string_view keyControl, std::string_view key, std::string_view val, typename DBAdapter::List &list, Result<Protocol> &result, uint32_t const iterations){
			using namespace queue_impl_;

			// seamlessly send value to output buffer...
			result.set(val);

			auto const score = getScore_(keyControl, key);

			if (iterations > ITERATIONS_IDLE){
				// update control key...
				logger<Logger::DEBUG>() << "SPOP: Update control key" << keyControl << "to" << score;

				[[maybe_unused]]
				hm4::TXGuard guard{ list };

				if (score.empty())
					hm4::erase(list, keyControl);
				else
					hm4::insert(list, keyControl, score);

				// delete data key...
				hm4::erase(list, key);
			}else{
				// delete data key...
				hm4::erase(list, key);
			}
		}

		static void finalizeTryAgain_(std::string_view keyControl, std::string_view key, typename DBAdapter::List &list, Result<Protocol> &result){
			auto const score = getScore_(keyControl, key);

			// update control key...
			logger<Logger::DEBUG>() << "SPOP: Update control key" << keyControl << "to" << key;

			if (score.empty())
				hm4::erase(list, keyControl);
			else
				hm4::insert(list, keyControl, score);

			// there is no valid data key.

			return result.set("");
		}

		static void finalizeEnd_(std::string_view keyControl, typename DBAdapter::List &list, Result<Protocol> &result){
			// delete control key...
			// better don't, because else there will be lots of syncs if new data come.
			if constexpr(0){
				logger<Logger::DEBUG>() << "SPOP: Remove control key" << keyControl;
				hm4::erase(list, keyControl);
			}

			// there is no valid data key.

			return result.set("");
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"spop",	"SPOP"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "queue";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SADD	,
				SPOP
			>(pack);
		}
	};



} // namespace


