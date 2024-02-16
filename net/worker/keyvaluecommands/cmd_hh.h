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
	} // namespace linear_curve_impl_



	template<class Protocol, class DBAdapter>
	struct HHADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		using MyRawHeavyHitter = heavy_hitter::RawHeavyHitter<hh_impl_::HH_KEY_SIZE>;

		// HHADD key slots item score

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace hh_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &key  = p[1];

			if (key.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (slots < MIN_SLOTS || slots > MAX_SLOTS)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const &item = p[3];

			if (item.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const score = from_string<uint64_t	>(p[4]);

			if (score == 0)
				return result.set_1();

			MyRawHeavyHitter const hh{ slots };

			const auto *pair = hm4::getPair_(*db, key, [&hh](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == hh.bytes())
					return & *it;
				else
					return nullptr;
			});

			if (pair && hm4::canInsertHintValSize(*db, pair, hh.bytes()))
				hm4::proceedInsertHintV<HHADD_Factory>(*db, pair, key, pair, hh, item, score);
			else
				hm4::insertV<HHADD_Factory>(*db, key, pair, hh, item, score);

			return result.set_1();
		}

	private:
		struct HHADD_Factory : hm4::PairFactory::IFactoryAction<1,1,HHADD_Factory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,HHADD_Factory>;

			HHADD_Factory(std::string_view const key, const Pair *pair, MyRawHeavyHitter const &hh, std::string_view const keySub, uint64_t const score) :
							Base::IFactoryAction	(key, hh.bytes(), pair),
							hh			(hh	),
							keySub			(keySub	),
							score			(score	){}

			void action(Pair *pair) const{
				using namespace hh_impl_;

				using Item = MyRawHeavyHitter::Item;

				Item *hh_data = reinterpret_cast<Item *>(pair->getValC());

				hh.add(hh_data, keySub, score);
			}

		private:
			MyRawHeavyHitter const	&hh;
			std::string_view	keySub;
			uint64_t		score;
		};


	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhadd",	"HHADD"
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

			if (key.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (slots < MIN_SLOTS || slots > MAX_SLOTS)
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

			if (key.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (slots < MIN_SLOTS || slots > MAX_SLOTS)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			MyRawHeavyHitter const hh{ slots };

			const auto *pair = hm4::getPair_(*db, key, [&hh](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == hh.bytes())
					return & *it;
				else
					return nullptr;
			});

			auto &container  = blob.container();
			auto &bcontainer = blob.bcontainer();

			if (pair == nullptr)
				return result.set_container(container);

			using Item = MyRawHeavyHitter::Item;

			const Item *hh_data = reinterpret_cast<const Item *>(pair->getValC());

			for(size_t i = 0; i < hh.size(); ++i){
				auto const &x = hh_data[i];

				if (!x.score)
					continue;

				auto const x_score = betoh(x.score);

				bcontainer.push_back();

				auto const item  = x.getItem();
				auto const score = to_string(x_score, bcontainer.back());

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
				HHADD		,
				HHRESERVE	,
				HHGET
			>(pack);
		}
	};



} // namespace

