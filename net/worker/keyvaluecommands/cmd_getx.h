#include "base.h"
#include "staticvector.h"
#include "mystring.h"



namespace net::worker::commands::GetX{

	namespace getx_impl_{

		constexpr static uint16_t MIN		= 10;
		constexpr static uint16_t ITERATIONS	= 1000;



		template<class It, class Container>
		void accumulateResults(uint16_t const maxResults, std::string_view const prefix, It it, It last, Container &data){
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



	template<class Protocol, class DBAdapter>
	struct GETX : Base<Protocol, DBAdapter>{
		constexpr inline static std::string_view name	= "getx";
		constexpr inline static std::string_view cmd[]	= {
			"getx",		"GETX"
		};

		Result operator()(Protocol &protocol, typename Protocol::StringVector const &p, DBAdapter &db, IOBuffer &buffer) const final{
			if (p.size() != 4)
				return Status::ERROR;

			auto const &key    = p[1];
			auto const count   = from_string<uint16_t>(p[2]);
			auto const &prefix = p[3];



			using namespace getx_impl_;

			MyVector data;
			data.reserve(SIZE);

			accumulateResults(
				std::clamp(count, MIN, ITERATIONS),
				prefix,
				db.search(key),
				std::end(db),
				data
			);



			protocol.response_strings(buffer, data);

			return {};
		}

	private:
		template<size_t Size>
		using VectorGETX = StaticVector<std::string_view,Size>;

		constexpr static size_t SIZE = 2 * getx_impl_::ITERATIONS + 1;

		using MyVector = VectorGETX<SIZE>;
	};




	template<class Protocol, class DBAdapter>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "getx";

		template<class Storage, class Map>
		static void load(Storage &s, Map &m){
			return registerCommands<Protocol, DBAdapter, Storage, Map,
				GETX
			>(s, m);
		}
	};



} // namespace

