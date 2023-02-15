#include "base.h"
#include "mystring.h"
#include "logger.h"

#include <algorithm>	// std::clamp

namespace net::worker::commands::ImmutableX{

	namespace getx_impl_{

		constexpr static uint32_t MIN			= 10;
		constexpr static uint32_t ITERATIONS		= (OutputBlob::ContainerSize - 1) / 2;



		template<bool Tail, class It, class Container, class ProjectionKey>
		void accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it, It eit, Container &container, ProjectionKey projKey){
			uint32_t iterations	= 0;
			uint32_t results	= 0;

			container.clear();

			for(;it != eit;++it){
				auto const &key = it->getKey();

				auto pkey = projKey(key);

				if (++iterations > ITERATIONS){
					if constexpr(Tail)
						container.emplace_back(pkey);

					return;
				}

				if (! prefix.empty() && ! same_prefix(prefix, key)){
					// no last key
					return;
				}

				if (! it->isValid(std::true_type()))
					continue;

				if (++results > maxResults){
					container.emplace_back(pkey);
					return;
				}

				auto const &val = it->getVal();

				container.emplace_back(pkey);
				container.emplace_back(val);
			}
		}

		template<class It, class Container>
		void accumulateResults(uint32_t const maxResults, std::string_view const prefix, It it, It eit, Container &container){
			return accumulateResults<true>(maxResults, prefix, it, eit, container, [](auto x){
				return x;
			});
		}
	} // namespace



	template<class Protocol, class DBAdapter>
	struct GETX : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;



			using namespace getx_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);

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
				count					,
				prefix					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getx",		"GETX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HGETALL : BaseRO<Protocol,DBAdapter>{
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
			if (p.size() != 2)
				return;



			using namespace getx_impl_;

			static_assert(OutputBlob::ContainerSize >= 2 * ITERATIONS + 1);



			auto const &keyN = p[1];

			if (keyN.empty())
				return;

			if (keyN.size() > MAX_KEY_SIZE)
				return;

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
				ITERATIONS				,
				key					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				blob.container				,
				proj
			);

			return result.set_container(blob.container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hgetall",	"HGETALL"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				GETX	,
				HGETALL
			>(pack);
		}
	};



} // namespace

