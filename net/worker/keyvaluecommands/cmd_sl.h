#include "base.h"
#include "slist.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

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

		template<class Protocol, class DBAdapter>
		void processADD(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, bool capacityMultiplier){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *pair = hm4::getPairPtr(*db, key);

			if (!pair)
				return result.set_container(container);

			using List = typename RawSList::List;

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
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

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
				SLGETALL	,
				SLCOUNT
			>(pack);
		}
	};



} // namespace

