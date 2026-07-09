#include "base.h"
#include "slist.h"
#include "logger.h"
#include "pair_vfactory.h"

#include "hashtable/easymap.h"
#include "hashtable/compactstorage.h"

namespace net::worker::commands::SL{

	namespace impl_{
		using namespace s_list;

		template<typename MyRawSList, typename Pair>
		[[nodiscard]]
		constexpr auto make_sl_(Pair *pair){
			static_assert(std::is_same_v<std::remove_const_t<Pair>, hm4::Pair>);

			auto *sl_data = hm4::getValAs<typename MyRawSList::List>(pair);

			return MyRawSList{ sl_data, pair->getValLen() };
		}

		[[nodiscard]]
		constexpr auto make_sl(hm4::Pair *pair){
			return make_sl_<RawSList>(pair);
		}

		[[nodiscard]]
		constexpr auto make_sl(const hm4::Pair *pair){
			return make_sl_<RawSListConst>(pair);
		}



		struct SLADDFactory : hm4::PairFactory::IFactoryAction<1,0,SLADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,0,SLADDFactory>;

			using It   = ParamContainer::const_iterator;

			SLADDFactory(std::string_view const key, const Pair *pair, size_t bytes, It begin, It end) :
							Base::IFactoryAction	(key, bytes, pair),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair){
				auto sl = make_sl(pair);

				for(auto itk = begin; itk != end; ++itk){
					if (sl.size() == OutputBlob::ContainerSize)
						break;

					sl.push(*itk);
				}
			}

		private:
			It	begin;
			It	end;
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
				if (const auto &item = *itk; !RawSListConst::isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			auto const bytes = [&](){
				// new pair
				if (!pair)
					return RawSListConst::bytes(std::begin(p) + varg, std::end(p));

				// existing pair

				auto const sl = make_sl(pair);

				return sl.bytes(std::begin(p) + varg, std::end(p), capacityMultiplier );
			}();

			SLADDFactory factory{ key, pair, bytes, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct SLADD : BaseCommandRW<Protocol,DBAdapter>{

		SLADD() : BaseCommandRW<Protocol,DBAdapter>("SLADD", std::begin(cmd__), std::end(cmd__)){}

		// SLADD key item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return impl_::processADD(p, db, result, true);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"sladd"		,	"SLADD"		,
			"slpush"	,	"SLPUSH"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SLADDPACK : BaseCommandRW<Protocol,DBAdapter>{

		SLADDPACK() : BaseCommandRW<Protocol,DBAdapter>("SLADDPACK", std::begin(cmd__), std::end(cmd__)){}

		// SLADD key item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return impl_::processADD(p, db, result, false);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"sladdpack"	,	"SLADDPACK"	,
			"slpushpack"	,	"SLPUSHPACK"	,
			"sladdshort"	,	"SLADDSHORT"	,
			"slpushshort"	,	"SLPUSHSHORT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SLGET : BaseCommandRO<Protocol,DBAdapter>{

		SLGET() : BaseCommandRO<Protocol,DBAdapter>("SLGET", std::begin(cmd__), std::end(cmd__)){}

		// SLGET key n

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set("");

			const auto n = from_string<uint64_t>(p[2]);

			auto const sl = make_sl(pair);

			std::string_view sv = "";

			auto f = [i = uint64_t{0}, &n, &sv](auto const &item) mutable{
				if (n == i){
					sv = item.getItem();
					return false;
				}

				++i;

				return true;
			};

			sl.for_each(f);

			return result.set(sv);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"slget",	"SLGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SLMGET : BaseCommandRO<Protocol,DBAdapter>{

		SLMGET() : BaseCommandRO<Protocol,DBAdapter>("SLMGET", std::begin(cmd__), std::end(cmd__)){}

		// SLMGET key val val val

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *pair = hm4::getPairPtr(*db, key);

			auto const varg = 2;

			if (!pair){
				container.assign(std::distance(std::begin(p) + varg, std::end(p)), "");

				return result.set_container(container);
			}

			auto const sl = make_sl(pair);

			auto const d = std::distance(std::begin(p) + varg, std::end(p));
			auto const s = sl.size();

			logger<Logger::NOTICE>() << "SLMGET" << "args" << d << "count" << s;

			if ( d <= 1 || s <= 1 || d * s < SEARCH_NAIVE)
				return processNaive__(result,      std::begin(p) + varg, std::end(p), container, sl);

			if (s < SEARCH_MINI)
				return processMini__(result, blob, std::begin(p) + varg, std::end(p), container, sl);
			else
				return processHuge__(result, blob, std::begin(p) + varg, std::end(p), container, sl);
		}

	private:
		constexpr static size_t HTMax  = OutputBlob::ParamContainerSize;
		constexpr static size_t HTSize = OutputBlob::ParamContainerSizeHTSize;

		constexpr static ssize_t SEARCH_NAIVE = 1024;
		constexpr static size_t  SEARCH_MINI  = HTMax;

		using ItemPtr	= const s_list::RawSListConst::Item *;

		template<typename T, size_t MaxItems, size_t Size>
		using MyStorage	= myhashtable::CompactStorage<T, MaxItems, Size, StaticVector>;

	private:
		static void processNaive__(Result<Protocol> &result,
					ParamContainer::iterator first, ParamContainer::iterator last,
						OutputBlob::Container &container, s_list::RawSListConst const &sl){

			logger<Logger::NOTICE>() << "SLMGET" << "quadratic";

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				bool found = false;

				auto f = [i = uint64_t{0}, &n, &container, &found](auto const &item) mutable{
					if (i == n){
						container.push_back(item.getItem());
						found = true;
					}

					++i;

					return true;
				};

				sl.for_each(f);

				if (!found)
					container.push_back();
			}

			return result.set_container(container);
		}

		static void processMini__(Result<Protocol> &result, OutputBlob &blob,
					ParamContainer::iterator first, ParamContainer::iterator last,
						OutputBlob::Container &container, s_list::RawSListConst const &sl){

			logger<Logger::NOTICE>() << "SLMGET" << "mini";

			using ItemContainer = StaticVector<ItemPtr,  HTMax>;

			auto &icontainer = blob.construct<ItemContainer>();

			auto f = [&icontainer](auto const &item){
				icontainer.push_back(&item);

				return true;
			};

			sl.for_each(f);

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				container.push_back(n < icontainer.size() ? (icontainer[n])->getItem() : "");
			}

			return result.set_container(container);
		}

		static void processHuge__(Result<Protocol> &result, OutputBlob &blob,
					ParamContainer::iterator first, ParamContainer::iterator last,
						OutputBlob::Container &container, s_list::RawSListConst const &sl){

			logger<Logger::NOTICE>() << "SLMGET" << "huge";

			using NContainer = StaticVector<uint64_t, HTMax>;

			using MyMap = myhashtable::EasyMap<uint64_t, ItemPtr, HTMax, HTSize, MyStorage>;

			auto &ncontainer	= blob.construct<NContainer>();
			auto &map		= blob.construct<MyMap>();

			for(auto itk = first; itk != last; ++itk){
				auto const n = from_string<uint64_t>(*itk);

				ncontainer.push_back(n);
				map.insert(n, nullptr);
			}

			auto f = [i = uint64_t{0}, &map](auto const &item) mutable{
				if (ItemPtr *p = map.find(i); p){
					*p = &item;
				}

				++i;

				return true;
			};

			sl.for_each(f);

			for(auto const &n : ncontainer){
				const auto *item = map.find(n);

				container.push_back(item ? (*item)->getItem() : "");
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"slmget",	"SLMGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SLGETALL : BaseCommandRO<Protocol,DBAdapter>{

		SLGETALL() : BaseCommandRO<Protocol,DBAdapter>("SLGETALL", std::begin(cmd__), std::end(cmd__)){}

		// SLGETALL key

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set_container(container);

			auto const sl = make_sl(pair);

			auto f = [&container](auto const &item){
				if (container.full())
					return false;

				container.push_back(item.getItem());

				return true;
			};

			sl.for_each(f);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"slgetall",	"SLGETALL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SLCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		SLCOUNT() : BaseCommandRO<Protocol,DBAdapter>("SLCOUNT", std::begin(cmd__), std::end(cmd__)){}

		// SLCOUNT key

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set_0();

			auto const sl = make_sl(pair);

			return result.set(
				uint64_t{ sl.size() }
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
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

