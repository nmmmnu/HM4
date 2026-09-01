#include "base.h"

#include "shared_rset_multi.h"
#include "stringtokenizer.h"

#include <algorithm>	// copy

namespace net::worker::commands::MultiIndex2{
	namespace impl_{

		constexpr size_t  MaxTokens	= 32;



		bool validateTokens(std::true_type, char delimiter, std::string_view tokens, OutputBlob::Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					continue;

				container.push_back(x);
			}

			if (container.empty())
				return false; // need to have at least one token

			std::sort(std::begin(container), std::end(container));

			#if 0
				container.erase(
					std::unique( std::begin(container), std::end(container) ),
					std::end(container)
				);
			#else
				// Quick fix for StaticVector et all

				if (auto it = std::unique(std::begin(container), std::end(container)); it != std::end(container))
					while (container.end() != it)
						container.pop_back();
			#endif

			return true;
		}

		bool validateTokens(std::false_type, char delimiter, std::string_view tokens, OutputBlob::Container &container){
			container.clear();

			StringTokenizer const tok{ tokens, delimiter };

			std::string_view prev;

			for(auto const &x : tok){
				if (container.full())
					return false; // no room for the token

				if (x.empty())
					return false;

				if (!prev.empty() && prev >= x)
					return false;

				container.push_back(x);

				prev = x;
			}

			if (container.empty())
				return false; // need to have at least one token

			return true;
		}

		size_t sizeTokens(OutputBlob::Container const &container){
			auto f = [](size_t sum, std::string_view sv){
				return sum + sv.size() + 1;
			};

			size_t const sum = std::accumulate(std::begin(container), std::end(container), size_t{ 0 }, f);

			return sum - 1;
		}



		template<typename DBAdapter>
		struct Decoder{
			constexpr static size_t bytes(){
				return 0;
			}

			template<typename IContainer, typename BContainer>
			bool operator()(std::string_view data,
						IContainer &icontainer, BContainer const &) const{

				return validateTokens(std::false_type{}, DBAdapter::SEPARATOR[0], data, icontainer);
			}
		};

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct IXMADD : BaseCommandRW<Protocol,DBAdapter>{

		IXMADD() : BaseCommandRW<Protocol,DBAdapter>("IXMADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMADD keyN keySub delimiter "words,words"

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const keyN		= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const keySub	= p[2];
			auto const delimiter	= p[3];
			auto const tokens	= p[4];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			if (keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!shared::rsetmulti::valid(keyN, keySub, shared::sortkey::keySortSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			to_string_buffer_t buffer;

			auto const keySort = shared::sortkey::makeHashKeySort(keySub, buffer);

			auto &tokenContainer = blob.construct<OutputBlob::Container>();

			if (!validateTokens(std::true_type{}, delimiter[0], tokens, tokenContainer))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			// ---------------------

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			const hm4::Pair *pair = nullptr;

			constexpr std::string_view key = ""; // will be replaces later.

			IXMADDFactory factory{ key, pair,
							tokenContainer, icontainer };

			[[maybe_unused]]
			bool const b = shared::rsetmulti::add(db, decoder,
							keyN, keySub, keySort,
								icontainer, bcontainer,
									factory);

			return result.set_1();
		}



		struct IXMADDFactory : hm4::PairFactory::IFactoryAction<0,0,IXMADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,0,IXMADDFactory>;

			IXMADDFactory(std::string_view const key, const Pair *pair,
											OutputBlob::Container &container,
											OutputBlob::Container &icontainer) :
							Base::IFactoryAction	(key, impl_::sizeTokens(container), pair),
							container		(container	),
							icontainer		(icontainer	){}

			void action(Pair *pair){
				char   *raw = pair->getValC();
				size_t size = 0;

				for(size_t i = 0; i < container.size(); ++i){
					auto const sv = container[i];

					concatenateRawBuffer_(raw + size, sv);		size += sv.size();

					if (i >= container.size() - 1)
						continue;

					*(raw + size) = DBAdapter::SEPARATOR[0];	size += 1;
				}

				// Decoder
				icontainer.clear();
				std::copy(std::begin(container), std::end(container), std::back_inserter(icontainer));

				for(size_t i = 0; i < icontainer.size(); ++i)
					std::cout << i << ' ' << icontainer[i] << '\n';
			}

			auto const &getIndexes() const{
				return icontainer;
			}

		private:
			OutputBlob::Container	&container;
			OutputBlob::Container	&icontainer;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmadd",	"IXMADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMREM : BaseCommandRW<Protocol,DBAdapter>{

		IXMREM() : BaseCommandRW<Protocol,DBAdapter>("IXMREM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// IXMREM keyN keySub...

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			auto const varg  = 2;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			auto const keyN		= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!shared::rsetmulti::valid(keyN, keySub, shared::sortkey::keySortSize))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const keySub	= *itk;

				to_string_buffer_t buffer;
				auto const keySort	= shared::sortkey::makeHashKeySort(keySub, buffer);

				[[maybe_unused]]
				bool const b = shared::rsetmulti::rem(db, decoder,
								keyN, keySub, keySort,
									icontainer, bcontainer);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmrem",	"IXMREM"
		};
	};



	template<class Protocol, class DBAdapter>
	struct IXMGETINDEXES : BaseCommandRO<Protocol,DBAdapter>{

		IXMGETINDEXES() : BaseCommandRO<Protocol,DBAdapter>("IXMGETINDEXES", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		// MHGETINDEXES keyN keySub
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const keyN		= p[1];
			auto const keySub	= p[2];

			using namespace impl_;

			if (!shared::rsetmulti::valid(keyN, keySub, shared::sortkey::keySortSize))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto &icontainer = blob.construct<OutputBlob::Container>();
			auto  bcontainer = std::nullptr_t{ nullptr }; // unused

			Decoder<DBAdapter> decoder;

			[[maybe_unused]]
			bool const b = shared::rsetmulti::getIndexes(db, decoder,
							keyN, keySub,
								icontainer, bcontainer);

			auto const &container = icontainer;

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ixmgetindexes",	"IXMGETINDEXES"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mindex2";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IXMADD		,
				IXMREM		,
				IXMGETINDEXES
			>(pack);
		}
	};

} // namespace net::worker::commands::MultiIndex2

