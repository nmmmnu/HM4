#include "base.h"
#include "idgenerator.h"
#include "mystring.h"
#include "logger.h"

namespace net::worker::commands::Queue{
	namespace queue_impl_{
		constexpr static uint32_t ITERATIONS			= 65'536;
		constexpr static uint32_t ITERATIONS_UPDATE_CONTROL_KEY	= 10;

		using MyIDGenerator = idgenerator::IDGeneratorTS_HEX;
	} // namespace



	template<class Protocol, class DBAdapter>
	struct SADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyIDGenerator = queue_impl_::MyIDGenerator;

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- MyIDGenerator::to_string_buffer_t_size
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			auto const &keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_KEY_SIZE)
				return;

			auto const &val = p[2];

			if (!hm4::Pair::isValValid(val))
				return;

			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			MyIDGenerator::to_string_buffer_t buffer;

			auto const &id = MyIDGenerator{}(buffer);

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, id);

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sadd",		"SADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SPOP : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyIDGenerator = queue_impl_::MyIDGenerator;

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- MyIDGenerator::to_string_buffer_t_size
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 2)
				return;

			// GET
			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_KEY_SIZE)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			auto it = db->find(key, std::false_type{});

			if (it == std::end(*db)){
				// case 1. it == end, no any data

				return result.set("");
			}

			if (it->getKey() == key){
				// case 2. there is a control key

				if (it->isOK()){
					// case 2.1. control key is valid, go to case 3

					logger<Logger::DEBUG>() << "SPOP: Control key" << key << "is valid";

					return collect_(
						key,
						db->find(it->getVal(), std::false_type{}),
						std::end(*db),
						*db,
						result
					);
				}else{
					// case 2.2. control key is NOT valid, go to case 3

					logger<Logger::DEBUG>() << "SPOP: Control key" << key << "is NOT valid";

					++it;
				}
			}

			// case 3, search next keys...

			logger<Logger::DEBUG>() << "SPOP: Next key for control key" << key;

			return collect_(
				key,
				it,
				std::end(*db),
				*db,
				result
			);
		}

	private:
		void collect_(std::string_view control_key, typename DBAdapter::List::iterator it, typename DBAdapter::List::iterator eit, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace queue_impl_;

			uint32_t iterations = 0;

			for(;it != eit;++it){
				auto const &key = it->getKey();

				if (! same_prefix(control_key, key)){
					logger<Logger::DEBUG>() << "SPOP: New prefix, done";
					return finalizeEnd_(control_key, list, result);
				}

				if (it->isOK()){
					logger<Logger::DEBUG>() << "SPOP: Valid key, done";
					auto const &val = it->getVal();
					return finalizeOK_(control_key, key, val, list, result, iterations);
				}

				if (++iterations > ITERATIONS){
					logger<Logger::DEBUG>() << "SPOP: Lots of iterations, done";
					return finalizeTryAgain_(control_key, key, list, result);
				}
			}

			// it == end
			return finalizeEnd_(control_key, list, result);
		}

		void finalizeOK_(std::string_view control_key, std::string_view key, std::string_view val, typename DBAdapter::List &list, Result<Protocol> &result, uint32_t const iterations) const{
			using namespace queue_impl_;

			// seamlessly send value to output buffer...
			result.set(val);

			if (iterations > ITERATIONS_UPDATE_CONTROL_KEY){
				// update control key...
				logger<Logger::DEBUG>() << "SPOP: Update control key" << control_key << "to" << key;
				hm4::insert(list, control_key, key);
			}

			// delete data key...
			hm4::erase(list, key);

			return;
		}

		void finalizeTryAgain_(std::string_view control_key, std::string_view key, typename DBAdapter::List &list, Result<Protocol> &result) const{
			// update control key...
			logger<Logger::DEBUG>() << "SPOP: Update control key" << control_key << "to" << key;
			hm4::insert(list, control_key, key);

			// there is no valid data key.

			return result.set("");
		}

		void finalizeEnd_(std::string_view control_key, typename DBAdapter::List &list, Result<Protocol> &result) const{
			// delete control key...
			// better don't, because else there will be lots of syncs if new data come.
			if constexpr(0){
				logger<Logger::DEBUG>() << "SPOP: Remove control key" << control_key;
				hm4::erase(list, control_key);
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


