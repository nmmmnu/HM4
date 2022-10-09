#include "base.h"
#include "mystring.h"

namespace net::worker::commands::GetX{

	namespace getx_impl_{

		constexpr static uint16_t MIN			= 10;
		constexpr static uint16_t ITERATIONS		= 1'000;
		constexpr static uint16_t ITERATIONS_DELX	= 1'000;



		template<bool Tail, class It, class OutputBlob, class ProjectionKey>
		void accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, OutputBlob &data, ProjectionKey projKey){
			uint16_t iterations	= 0;
			uint16_t results	= 0;

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
		void accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, OutputBlob &data){
			return accumulateResults<true>(maxResults, prefix, it, data, [](auto x){
				return x;
			});
		}
	}



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
			// because this value is great than max uint16_t,
			// the converted value will go to 0, then to MIN.

			auto myClamp = [](auto a){
				return static_cast<uint16_t>(
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

			auto const key = concatenateBuffer(blob.string_key, keyN, DBAdapter::SEPARATOR);

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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3)
				return Result::error();

			auto const &key    = p[1];
			auto const &prefix = p[2];

			if (prefix.empty())
				return Result::error();



			using namespace getx_impl_;

			uint16_t iterations = 0;

			blob.string_key = "";

			std::vector<std::string> container;
			container.reserve(ITERATIONS_DELX);

			auto it = db.find(key, std::false_type{});

			for(;it;++it){
				auto const key = it->getKey();

				if (! same_prefix(prefix, key))
					break;

				if (++iterations > ITERATIONS_DELX){
					blob.string_key = key;
					break;
				}

				if (! it->isValid(std::true_type{}))
					continue;

				container.emplace_back(key);

				// db.del(it->getKey());
			}

			for(auto const &x : container){
				db.del(x);
			}

			if (!blob.string_key.empty())
				return Result::ok(blob.string_key);
			else
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

