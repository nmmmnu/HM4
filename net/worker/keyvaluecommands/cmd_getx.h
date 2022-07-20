#include "base.h"
#include "mystring.h"



namespace net::worker::commands::GetX{

	namespace getx_impl_{

		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t ITERATIONS	= 1000;



		template<class It, class OutputBlob>
		void accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, It last, OutputBlob &data){
			uint16_t iterations	= 0;
			uint16_t results	= 0;

			for(; it != last; ++it){
				auto const &key = it->getKey();

				if (++iterations > ITERATIONS){
					data.emplace_back(key);
					return;
				}

				if (! prefix.empty() && ! same_prefix(prefix, key)){
					// no last key
					return;
				}

				if (! it->isValid(std::true_type{}))
					continue;

				if (++results > maxResults){
					data.emplace_back(key);
					return;
				}

				auto const &val = it->getVal();

				data.emplace_back(key);
				data.emplace_back(val);
			}
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
			// suppose he enters 1'000'000'000.
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
				count		,
				prefix		,
				db.search(key)	,
				std::end(db)	,
				container
			);

			return Result::ok_container(container);
		}
	};




	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				GETX
			>(pack);
		}
	};



} // namespace

