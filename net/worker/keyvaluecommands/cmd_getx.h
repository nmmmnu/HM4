#include "base.h"
#include "mystring.h"



namespace net::worker::commands::GetX{

	namespace getx_impl_{

		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t ITERATIONS	= 1000;



		template<class It, class OutputContainer>
		void accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, It last, OutputContainer &data){
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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputContainer &container) const final{
			if (p.size() != 4)
				return Result::error();

			auto const &key    = p[1];
			auto const count   = from_string<uint16_t>(p[2]);
			auto const &prefix = p[3];



			using namespace getx_impl_;

			static_assert(OutputContainerSize >= 2 * ITERATIONS + 1);

			container.clear();

			accumulateResults(
				std::clamp(count, MIN, ITERATIONS),
				prefix,
				db.search(key),
				std::end(db),
				container
			);

			return Result::ok(&container);
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

