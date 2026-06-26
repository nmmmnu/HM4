#include "base.h"
#include "slist.h"
#include "logger.h"
#include "pair_vfactory.h"
//#include "shared_hint.h"

#include "hashtable/easyset.h"
#include "hashtable/easymap.h"
#include "hashtable/compactstorage.h"

namespace net::worker::commands::SL{

	namespace sl_impl_{
		using namespace s_list;



		struct SLADDFactory : hm4::PairFactory::IFactoryAction<1,0,SLADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,0,SLADDFactory>;

			using It   = ParamContainer::const_iterator;

			SLADDFactory(std::string_view const key, const Pair *pair, size_t bytes, It begin, It end) :
							Base::IFactoryAction	(key, bytes, pair),
							sl			(bytes	),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair){
				using List = typename RawSList::List;

				auto *sl_data = hm4::getValAs<List>(pair);

				for(auto itk = begin; itk != end; ++itk){
					if (sl.size(*sl_data) == OutputBlob::ContainerSize)
						break;

					auto const &item = *itk;

					sl.push(*sl_data, item);
				}
			}

		private:
			RawSList	sl;
			It			begin;
			It			end;
		};

		// SLADD key val val val

		template<class Protocol, class DBAdapter>
		void processADD(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, bool capacityMultiplier){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &item = *itk; !RawSList::isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			auto const bytes = [&](){
				// new pair
				if (!pair)
					return RawSList::bytes(std::begin(p) + varg, std::end(p));

				// existing pair

				auto const capacity = pair->getVal().size();

				RawSList sl{ capacity };

				using List = typename RawSList::List;

				const auto &sl_data = *hm4::getValAs<List>(pair);

				return sl.bytes(sl_data, std::begin(p) + varg, std::end(p), capacityMultiplier );
			}();

			SLADDFactory factory{ key, pair, bytes, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	} // namespace sl_impl_



	template<class Protocol, class DBAdapter>
	struct SLADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLADD key item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return sl_impl_::processADD(p, db, result, true);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sladd"		,	"SLADD"		,
			"slpush"	,	"SLPUSH"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SLADDPACK : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLADD key item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return sl_impl_::processADD(p, db, result, false);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"sladdpack"	,	"SLADDPACK"	,
			"slpushpack"	,	"SLPUSHPACK"	,
			"sladdshort"	,	"SLADDSHORT"	,
			"slpushshort"	,	"SLPUSHSHORT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SLGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLGET key n

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace s_list;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set("");

			const auto n = from_string<uint64_t>(p[2]);

			auto const capacity = pair->getVal().size();

			RawSList sl{ capacity };

			using List = typename RawSList::List;

			const auto &sl_data = *hm4::getValAs<List>(pair);

			uint64_t i = 0;
			std::string_view sv = "";

			sl.for_each(sl_data, [&n, &i, &sv](auto const &item){
				if (n == i){
					sv = item.getItem();
					return false;
				}

				++i;

				return true;
			});

			return result.set(sv);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"slget",	"SLGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SLMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLMGET key val val val

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace s_list;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<SContainer>();

			const auto *pair = hm4::getPairPtr(*db, key);

			auto const varg = 2;

			if (!pair){
				container.assign(std::distance(std::begin(p) + varg, std::end(p)), "");

				return result.set_container(container);
			}

			auto const capacity = pair->getVal().size();

			RawSList sl{ capacity };

			using List = typename RawSList::List;

			const auto &sl_data = *hm4::getValAs<List>(pair);

			auto const d = std::distance(std::begin(p) + varg, std::end(p));
			auto const s = sl.size(sl_data);

			logger<Logger::NOTICE>() << "SLMGET" << "args" << d << "count" << s;

			if ( d <= 1 || s <= 1 || d * s < SEARCH_NAIVE)
				return processNaive__(result,      std::begin(p) + varg, std::end(p), container, sl, sl_data);

			if (s < SEARCH_MINI)
				return processMini__(result, blob, std::begin(p) + varg, std::end(p), container, sl, sl_data);
			else
				return processHuge__(result, blob, std::begin(p) + varg, std::end(p), container, sl, sl_data);
		}

	private:
		constexpr static size_t HTMax  = OutputBlob::ParamContainerSize;
		constexpr static size_t HTSize = OutputBlob::ParamContainerSizeHTSize;

		constexpr static ssize_t SEARCH_NAIVE = 1024;
		constexpr static size_t  SEARCH_MINI  = HTMax;


		using SContainer = StaticVector<std::string_view, HTMax>;
		using NContainer = StaticVector<uint64_t, HTMax>;

		using MySet = myhashtable::EasySet<uint64_t,                   HTMax, HTSize, myhashtable::CompactStorage>;
		using MyMap = myhashtable::EasyMap<uint64_t, std::string_view, HTMax, HTSize, myhashtable::CompactStorage>;

	private:
		static void processNaive__(Result<Protocol> &result,
					ParamContainer::iterator first, ParamContainer::iterator last,
						SContainer &container, s_list::RawSList const &sl, typename s_list::RawSList::List const &sl_data){

			logger<Logger::NOTICE>() << "SLMGET" << "quadratic";

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				bool found = false;

				sl.for_each(sl_data, [i = uint64_t{0}, &n, &container, &found](auto const &item) mutable{
					if (i == n){
						container.push_back(item.getItem());
						found = true;
					}

					++i;

					return true;
				});

				if (!found)
					container.push_back();
			}

			return result.set_container(container);
		}

		static void processMini__(Result<Protocol> &result, OutputBlob &blob,
					ParamContainer::iterator first, ParamContainer::iterator last,
						SContainer &container, s_list::RawSList const &sl, typename s_list::RawSList::List const &sl_data){

			logger<Logger::NOTICE>() << "SLMGET" << "mini";

			auto &map = blob.construct<SContainer>();

			sl.for_each(sl_data, [&map](auto const &item){
				map.push_back(item.getItem());

				return true;
			});

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				container.push_back(n < map.size() ? map[n] : "");
			}

			return result.set_container(container);
		}

		static void processHuge__(Result<Protocol> &result, OutputBlob &blob,
					ParamContainer::iterator first, ParamContainer::iterator last,
						SContainer &container, s_list::RawSList const &sl, typename s_list::RawSList::List const &sl_data){

			logger<Logger::NOTICE>() << "SLMGET" << "huge";

			auto &ncontainer = blob.construct<NContainer>();

			auto &set        = blob.construct<MySet>();
			auto &map        = blob.construct<MyMap>();

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				ncontainer.push_back(n);
				set.insert(n);
			}

			sl.for_each(sl_data, [i = uint64_t{0}, &set, &map](auto const &item) mutable{
				if (set.exists(i)){
					[[maybe_unused]]
					auto const u = map.insert(i, item.getItem());
				}

				++i;

				return true;
			});

			for(auto const &n : ncontainer){
				const auto *s = map.find(n);

				container.push_back(s ? *s : "");
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"slmget",	"SLMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SLGETALL : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLGETALL key

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace s_list;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set_container(container);

			auto const capacity = pair->getVal().size();

			RawSList sl{ capacity };

			using List = typename RawSList::List;

			const auto &sl_data = *hm4::getValAs<List>(pair);

			sl.for_each(sl_data, [&container](auto const &item){
				if (container.full())
					return false;

				container.push_back(item.getItem());

				return true;
			});

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"slgetall",	"SLGETALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SLCOUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// SLCOUNT key

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set_0();

			using namespace s_list;

			using List = typename RawSList::List;

			auto const capacity = pair->getVal().size();

			RawSList sl{ capacity };

			using List = typename RawSList::List;

			const auto &sl_data = *hm4::getValAs<List>(pair);

			return result.set(
				uint64_t{ sl.size(sl_data) }
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"slcount",	"SLCOUNT",
			"sllen",	"SLLEN"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "sl";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SLADD		,
				SLADDPACK	,
				SLGET		,
				SLMGET		,
				SLGETALL	,
				SLCOUNT
			>(pack);
		}
	};



} // namespace

