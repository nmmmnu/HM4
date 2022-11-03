#include "base.h"
#include "idgenerator.h"
#include "mystring.h"
#include "logger.h"

namespace net::worker::commands::Queue{
	namespace getx_impl_{
		constexpr static uint32_t ITERATIONS			= 65'536;
		constexpr static uint32_t ITERATIONS_UPDATE_CONTROL_KEY	= 10;

		using MyIDGenerator = idgenerator::IDGeneratorTS_HEX;
	} // namespace



	template<class DBAdapter>
	struct SADD : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "sadd";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"sadd",		"SADD"
		};

		using MyIDGenerator = getx_impl_::MyIDGenerator;

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- MyIDGenerator::to_string_buffer_t_size
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			auto const &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			if (keyN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			MyIDGenerator::to_string_buffer_t buffer;

			auto const &id = MyIDGenerator{}(buffer);

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, id);

			db.set(key, val, exp);

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct SPOP : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "spop";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"spop",	"SPOP"
		};

		using MyIDGenerator = getx_impl_::MyIDGenerator;

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- MyIDGenerator::to_string_buffer_t_size
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 2)
				return Result::error();

			// GET
			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			if (keyN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			auto it = db.find(key, std::false_type{});

			if (!it){
				// case 1. it == end, no any data

				return Result::ok("");
			}

			if (it->getKey() == key){
				// case 2. there is a control key

				if (it->isValid(std::true_type{})){
					// case 2.1. control key is valid, go to case 3

					log__<LogLevel::DEBUG>("Control key", key, "is valid");

					return collect_(
						key,
						db.find(it->getVal(), std::false_type{}),
						db,
						blob
					);
				}else{
					// case 2.2. control key is NOT valid, go to case 3

					log__<LogLevel::DEBUG>("Control key", key, "is NOT valid");

					++it;
				}
			}

			// case 3, search next keys...

			log__<LogLevel::DEBUG>("Next key for control key", key);

			return collect_(
				key,
				it,
				db,
				blob
			);
		}

	private:
		Result collect_(std::string_view control_key, typename DBAdapter::IteratorAdapter it, DBAdapter &db, OutputBlob &blob) const{
			using namespace getx_impl_;

			uint32_t iterations = 0;

			for(;it;++it){
				auto const &key = it->getKey();

				if (! same_prefix(control_key, key)){
					log__<LogLevel::DEBUG>("New prefix, done");
					return finalizeEnd_(control_key, db, blob);
				}

				if (it->isValid(std::true_type{})){
					log__<LogLevel::DEBUG>("Valid key, done");
					auto const &val = it->getVal();
					return finalizeOK_(control_key, key, val, db, blob, iterations);
				}

				if (++iterations > ITERATIONS){
					log__<LogLevel::DEBUG>("Lots of iterations, done");
					return finalizeTryAgain_(control_key, key, db, blob);
				}
			}

			// it == end
			return finalizeEnd_(control_key, db, blob);
		}

		Result finalizeOK_(std::string_view control_key, std::string_view key, std::string_view val, DBAdapter &db, OutputBlob &blob, uint32_t const iterations) const{
			using namespace getx_impl_;

			// store value, because we will delete it...
			blob.string = val;

			if (iterations > ITERATIONS_UPDATE_CONTROL_KEY){
				// update control key...
				db.set(control_key, key);
			}

			// delete data key...
			db.del(key);

			return Result::ok(blob.string);
		}

		Result finalizeTryAgain_(std::string_view control_key, std::string_view key, DBAdapter &db, OutputBlob &) const{
			// update control key...
			db.set(control_key, key);

			// there is no valid data key.

			return Result::ok("");
		}

		Result finalizeEnd_(std::string_view control_key, DBAdapter &db, OutputBlob &) const{
			// delete control key...
			// better don't, because else there will be lots of syncs if new data come.
			if constexpr(0){
				db.del(control_key);
			}

			// there is no valid data key.

			return Result::ok("");
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "queue";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				SADD	,
				SPOP
			>(pack);
		}
	};



} // namespace


