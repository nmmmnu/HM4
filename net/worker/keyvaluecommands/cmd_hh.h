#include "base.h"
#include "heavyhitter.h"
#include "logger.h"
#include "pair_vfactory.h"

namespace net::worker::commands::HH{
	namespace hh_impl_{
		constexpr char   DELIMITER	= ',';

		constexpr size_t  HH_KEY_SIZE	=  63;

		constexpr uint8_t MIN_SLOTS	=   1;
		constexpr uint8_t MAX_SLOTS	= 100;

		constexpr bool isSlotsValid(uint8_t slots){
			return slots >= MIN_SLOTS && slots <= MAX_SLOTS;
		}

		constexpr bool isItemValid(std::string_view item){
			return item.size() >=1 && item.size() <= 63;
		}



		using MyRawHeavyHitter = heavy_hitter::RawHeavyHitter<hh_impl_::HH_KEY_SIZE>;



		template<typename It, bool Up>
		struct HHADDFactory : hm4::PairFactory::IFactoryAction<1,1,HHADDFactory<It, Up> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,HHADDFactory>;

			HHADDFactory(std::string_view const key, const Pair *pair, MyRawHeavyHitter hh, It begin, It end) :
							Base::IFactoryAction	(key, hh.bytes(), pair),
							hh			(hh	),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair) const{
				using Item = MyRawHeavyHitter::Item;

				Item *hh_data = reinterpret_cast<Item *>(pair->getValC());

				for(auto itk = begin; itk != end; itk += 2){
					auto const &item = *itk;
					auto const score = from_string<int64_t>(*std::next(itk));

					hh.add<Up>(hh_data, item, score);
				}
			}

		private:
			MyRawHeavyHitter	hh;
			It			begin;
			It			end;
		};



		template<class Protocol, class DBAdapter, bool Up>
		void do_hh_incr_decr(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, std::bool_constant<Up>){
			if (p.size() < 5 || p.size() % 2 == 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &item = *itk; !isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			MyRawHeavyHitter const hh{ slots };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, hh.bytes());

			using MyHHADDFactory = HHADDFactory<ParamContainer::iterator, Up>;

			if (pair && hm4::canInsertHintValSize(*db, pair, hh.bytes()))
				hm4::proceedInsertHintV<MyHHADDFactory>(*db, pair,	key, pair, hh, std::begin(p) + varg, std::end(p));
			else
				hm4::insertV<MyHHADDFactory>(*db,			key, pair, hh, std::begin(p) + varg, std::end(p));

			return result.set_1();
		}

	} // namespace hh_impl_



	template<class Protocol, class DBAdapter>
	struct HHINCR : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHADD key slots item score item score

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace hh_impl_;

			return do_hh_incr_decr(p, db, result, std::true_type{});
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhincr",	"HHINCR"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HHDECR : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHADD key slots item score item score

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace hh_impl_;

			return do_hh_incr_decr(p, db, result, std::false_type{});
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhdecr",	"HHDECR"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HHRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyRawHeavyHitter = heavy_hitter::RawHeavyHitter<hh_impl_::HH_KEY_SIZE>;

		// HHRESERVE key slots

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace hh_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			MyRawHeavyHitter const hh{ slots };

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, hh.bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhreserve",	"HHRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HHGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyRawHeavyHitter = heavy_hitter::RawHeavyHitter<hh_impl_::HH_KEY_SIZE>;

		// HHGET key slots

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace hh_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			MyRawHeavyHitter const hh{ slots };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, hh.bytes());

			auto &container  = blob.container();

			if (pair == nullptr)
				return result.set_container(container);

			auto &bcontainer = blob.bcontainer();

			using Item = MyRawHeavyHitter::Item;

			const Item *hh_data = reinterpret_cast<const Item *>(pair->getValC());

			for(size_t i = 0; i < hh.size(); ++i){
				auto const &x = hh_data[i];

				if (!x.valid())
					continue;

				bcontainer.push_back();

				auto const item  = x.getItem();
				auto const score = to_string(x.getScore(), bcontainer.back());

				container.push_back(item);
				container.push_back(score);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhget",	"HHGET"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hh";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				HHINCR		,
				HHDECR		,
				HHRESERVE	,
				HHGET
			>(pack);
		}
	};



} // namespace

