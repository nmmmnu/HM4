#include "base.h"
#include "heavyhitter.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

namespace net::worker::commands::HH{
	namespace hh_impl_{
		constexpr char   DELIMITER	= ',';

		constexpr uint8_t MIN_SLOTS	=   1;
		constexpr uint8_t MAX_SLOTS	= 200;

		constexpr bool isSlotsValid(uint8_t slots){
			return slots >= MIN_SLOTS && slots <= MAX_SLOTS;
		}



		template<typename T>
		struct type_identity{
			// C++20 std::type_identity
			using type = T;
		};

		template<typename F>
		auto type_dispatch(size_t const t, F f){
			using namespace heavy_hitter;

			switch(t){
			case  16 : return f(type_identity<RawHeavyHitter16	>{});
			case  32 : return f(type_identity<RawHeavyHitter32	>{});
			case  40 : return f(type_identity<RawHeavyHitter40	>{});
			case  64 : return f(type_identity<RawHeavyHitter64	>{});
			case 128 : return f(type_identity<RawHeavyHitter128	>{});
			case 256 : return f(type_identity<RawHeavyHitter256	>{});
			default  : return f(type_identity<std::nullptr_t	>{});
			}
		}



		template<typename It, bool Up, class MyRawHeavyHitter>
		struct HHADDFactory : hm4::PairFactory::IFactoryAction<1,1,HHADDFactory<It, Up, MyRawHeavyHitter> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,HHADDFactory>;

			HHADDFactory(std::string_view const key, const Pair *pair, MyRawHeavyHitter hh, It begin, It end) :
							Base::IFactoryAction	(key, hh.bytes(), pair),
							hh			(hh	),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair){
				this->result = action_(pair);
			}

			constexpr bool getResult() const{
				return result;
			}

		private:
			bool action_(Pair *pair) const{
				using Item = typename MyRawHeavyHitter::Item;

				auto *hh_data = hm4::getValAs<Item>(pair);

				bool result = false;

				for(auto itk = begin; itk != end; itk += 2){
					auto const &item = *itk;
					auto const score = from_string<int64_t>(*std::next(itk));

					result |= hh. template add<Up>(hh_data, item, score);
				}

				return result;
			}

		private:
			MyRawHeavyHitter	hh;
			It			begin;
			It			end;
			bool			result = false;
		};



		template<class Protocol, class DBAdapter, bool Up>
		void do_hh_incr_decr(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, std::bool_constant<Up>){
			auto const varg = 4;

			if (p.size() < 6 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRawHeavyHitter = T;

					MyRawHeavyHitter const hh{ slots };

					for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
						if (const auto &item = *itk; !hh.isItemValid(item))
							return result.set_error(ResultErrorMessages::EMPTY_KEY);

					const auto *pair = hm4::getPairPtrWithSize(*db, key, hh.bytes());

					using MyHHADDFactory = HHADDFactory<ParamContainer::iterator, Up, MyRawHeavyHitter>;

					MyHHADDFactory factory{ key, pair, hh, std::begin(p) + varg, std::end(p) };

					insertHintVFactory(pair, *db, factory);

					return result.set(
						factory.getResult()
					);
				}
			};

			return type_dispatch(bytes, f);
		}

	} // namespace hh_impl_



	template<class Protocol, class DBAdapter>
	struct HHINCR : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHADD key slots bytes item score item score

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
	struct HHDECR : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHADD key slots bytes item score item score

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
	struct HHRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHRESERVE key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace hh_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRawHeavyHitter = T;

					MyRawHeavyHitter const hh{ slots };

					hm4::insertV<hm4::PairFactory::Reserve>(*db, key, hh.bytes());

					return result.set_1();
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hhreserve",	"HHRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HHGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// HHGET key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace hh_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRawHeavyHitter = T;

					MyRawHeavyHitter const hh{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, hh.bytes());

					return process_(hh, pair, result, blob);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyRawHeavyHitter>
		void process_(MyRawHeavyHitter const &hh, const hm4::Pair *pair, Result<Protocol> &result, OutputBlob &blob){
			auto &container = blob.container();

			if (pair == nullptr)
				return result.set_container(container);

			auto &bcontainer = blob.bcontainer();

			using Item = typename MyRawHeavyHitter::Item;

			const auto *hh_data = hm4::getValAs<Item>(pair);

			for(size_t i = 0; i < hh.size(); ++i)
				if(auto const &x = hh_data[i]; x){
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

