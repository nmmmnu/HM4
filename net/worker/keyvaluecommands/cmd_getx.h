#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

namespace net::worker::commands::GetX{

	namespace getx_impl_{

		constexpr static uint32_t MIN			= 10;
		constexpr static uint32_t ITERATIONS		= (OutputBlob::ContainerSize - 1) / 2;
		constexpr static uint32_t ITERATIONS_DELX	=  OutputBlob::ContainerSize;
		constexpr static uint32_t PASSES_DELX		= 3;



		template<bool Tail, class It, class OutputBlob, class ProjectionKey>
		void accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it, OutputBlob &data, ProjectionKey projKey){
			uint32_t iterations	= 0;
			uint32_t results	= 0;

			for(;it;++it){
				auto const &key = it->getKey();

				auto pkey = projKey(key);

				if (++iterations > ITERATIONS){
					if constexpr(Tail)
						data.emplace_back(pkey);

					return;
				}

				if (! prefix.empty() && ! same_prefix(prefix, key)){
					// no last key
					return;
				}

				if (! it->isValid(std::true_type()))
					continue;

				if (++results > maxResults){
					data.emplace_back(pkey);
					return;
				}

				auto const &val = it->getVal();

				data.emplace_back(pkey);
				data.emplace_back(val);
			}
		}

		template<class It, class OutputBlob>
		void accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it, OutputBlob &data){
			return accumulateResults<true>(maxResults, prefix, it, data, [](auto x){
				return x;
			});
		}
	} // namespace



	template<class DBAdapter>
	struct GETX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "getx";
		constexpr inline static std::string_view cmd[]	= {
			"getx",		"GETX"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 4)
				return Result::error();



			using namespace getx_impl_;

			auto &container = blob.container;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);
			container.clear();

			// using uint64_t from the user, allow more user-friendly behavour.
			// suppose he / she enters 1'000'000'000.
			// because this value is great than max uint32_t,
			// the converted value will go to 0, then to MIN.

			auto myClamp = [](auto a){
				return static_cast<uint32_t>(
					std::clamp<uint64_t>(a, MIN, ITERATIONS)
				);
			};



			auto const &key    = p[1];
			auto const count   = myClamp( from_string<uint64_t>(p[2]) );
			auto const &prefix = p[3];

			accumulateResults(
				count				,
				prefix				,
				db.find(key, std::false_type{})	,
				container
			);

			return Result::ok_container(container);
		}
	};



	template<class DBAdapter>
	struct HGETALL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hgetall";
		constexpr inline static std::string_view cmd[]	= {
			"hgetall",	"HGETALL"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 2)
				return Result::error();



			using namespace getx_impl_;

			auto &container = blob.container;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);
			container.clear();



			auto const &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			if (keyN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR);

			auto const key_size = key.size();

			auto proj = [key_size](std::string_view x) -> std::string_view{
				if (key_size <= x.size())
					return x.substr(key_size);
				else
					return x;
			};

			bool const no_tail = false;

			accumulateResults<no_tail>(
				ITERATIONS			,
				key				,
				db.find(key, std::false_type{})	,
				container			,
				proj
			);

			return Result::ok_container(container);
		}
	};



	template<class DBAdapter>
	struct DELX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "delx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"delx",		"DELX"
		};

		using Container = StaticVector<std::string_view, getx_impl_::ITERATIONS_DELX>;

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3)
				return Result::error();

			auto const &key    = p[1];
			auto const &prefix = p[2];

			if (prefix.empty())
				return Result::error();



			using namespace getx_impl_;

			auto &container = blob.container;
			container.reserve(ITERATIONS_DELX);

			uint8_t check_passes = 0;

		// label for goto
		start:
			std::string_view string_key = scanKeysAndDeleteInPlace_(db, prefix, key, container);

			for(auto const &x : container){
				db.del(x);

				if (db.mutable_size() == 0){
					// list just flushed.
					++check_passes;

					log__<LogLevel::WARNING>("DELX", "Restart because of table flush", check_passes);

					if (check_passes < PASSES_DELX)
						goto start;
					else
						return scanForLastKey_(db, prefix, key);
				}
			}

			return Result::ok(string_key);
		}

	private:
		template<class Container>
		std::string_view scanKeysAndDeleteInPlace_(DBAdapter &db, std::string_view prefix, std::string_view key, Container &container) const{
			using namespace getx_impl_;

			container.clear();

			uint32_t iterations = 0;

			for(auto it = db.find(key, std::false_type{});it;++it){
				auto const key = it->getKey();

				if (! same_prefix(prefix, key))
					break;

				if (++iterations > ITERATIONS_DELX){
					log__<LogLevel::NOTICE>("DELX", "iterations", iterations, key);
					return key;
				}

				if (! it->isValid(std::true_type{}))
					continue;

				if (db.canUpdateWithHint(& *it))
					db.delHint(& *it);
				else
					container.emplace_back(key);
			}

			return {};
		}

		Result scanForLastKey_(DBAdapter &db, std::string_view prefix, std::string_view key) const{
			using namespace getx_impl_;

			uint32_t iterations = 0;

			for(auto it = db.find(key, std::false_type{});it;++it){
				auto const key = it->getKey();

				if (! same_prefix(prefix, key))
					break;

				if (++iterations > ITERATIONS_DELX){
					log__<LogLevel::NOTICE>("DELX", "iterations", iterations, key);
					return Result::ok(key);
				}

				if (! it->isValid(std::true_type{}))
					continue;

				return Result::ok(key);
			}

			return Result::ok();
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				GETX	,
				HGETALL	,
				DELX
			>(pack);
		}
	};



} // namespace

