#include "base.h"
#include "reservoirsampling.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

namespace net::worker::commands::RS{
	namespace rs_impl_{
		constexpr uint16_t MIN_SLOTS	=    1;
		constexpr uint16_t MAX_SLOTS	= 4096;

		constexpr bool isSlotsValid(uint16_t slots){
			return slots >= MIN_SLOTS && slots <= MAX_SLOTS;
		}

		template<typename T>
		struct type_identity{
			// C++20 std::type_identity
			using type = T;
		};

		template<typename F>
		auto type_dispatch(size_t const t, F f){
			using namespace reservoir_sampling;

			switch(t){
			case  16 : return f(type_identity<RawReservoirSampling16	>{});
			case  32 : return f(type_identity<RawReservoirSampling32	>{});
			case  40 : return f(type_identity<RawReservoirSampling40	>{});
			case  64 : return f(type_identity<RawReservoirSampling64	>{});
			case 128 : return f(type_identity<RawReservoirSampling128	>{});
			case 256 : return f(type_identity<RawReservoirSampling256	>{});
			default  : return f(type_identity<std::nullptr_t		>{});
			}
		}

	} // namespace rs_impl_



	template<class Protocol, class DBAdapter>
	struct RSADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RSADD key slots bytes item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rs_impl_;

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyReservoirSampling = T;

					MyReservoirSampling const rs{ slots };

					return process_(rs, key, p, *db, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<typename MyReservoirSampling>
		void process_(MyReservoirSampling const &rs, std::string_view const key, ParamContainer const &p, typename DBAdapter::List &list, Result<Protocol> &result){
			auto const varg = 4;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &item = *itk; !rs.isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(list, key, rs.bytes());

			using MyRSADDFactory = RSADDFactory<ParamContainer::iterator, MyReservoirSampling>;

			MyRSADDFactory factory{ key, pair, rs, std::begin(p) + varg, std::end(p), rand64 };

			insertHintVFactory(pair, list, factory);

			return result.set(
				factory.getResult()
			);
		}

	private:
		template<typename It, typename MyReservoirSampling>
		struct RSADDFactory : hm4::PairFactory::IFactoryAction<1,1,RSADDFactory<It, MyReservoirSampling> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,RSADDFactory>;

			RSADDFactory(std::string_view const key, const Pair *pair, MyReservoirSampling rs, It begin, It end, std::mt19937_64 &rand64) :
							Base::IFactoryAction	(key, rs.bytes(), pair),
							rs			(rs	),
							begin			(begin	),
							end			(end	),
							rand64			(rand64	){}

			void action(Pair *pair){
				this->result = action_(pair);
			}

			constexpr bool getResult() const{
				return result;
			}

		private:
			bool action_(Pair *pair){
				using List = typename MyReservoirSampling::List;

				auto *rs_data = reinterpret_cast<List *>(pair->getValC());

				bool result = false;

				using Added = typename MyReservoirSampling::Added;

				auto chk = [](Added result){
					switch(result){
					case Added::INIT	:
					case Added::YES		: return true;
					case Added::NO		: return false;
					}

					return false;
				};

				for(auto itk = begin; itk != end; ++itk){
					auto const &item = *itk;

					result |= chk( rs.add(rs_data, item, rand64) );
				}

				return result;
			}

		private:
			MyReservoirSampling	rs;
			It			begin;
			It			end;
			bool			result = false;
			std::mt19937_64		&rand64;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rsadd",	"RSADD"
		};

		std::mt19937_64 rand64{ static_cast<uint32_t>(time(nullptr)) };
	};



	template<class Protocol, class DBAdapter>
	struct RSRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RSRESERVE key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rs_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyReservoirSampling = T;

					MyReservoirSampling const rs{ slots };

					hm4::insertV<hm4::PairFactory::Reserve>(*db, key, rs.bytes());

					return result.set_1();
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rsreserve",	"RSRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RSGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RSGET key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace rs_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyReservoirSampling = T;

					MyReservoirSampling const rs{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, rs.bytes());

					return process_(rs, pair, result, blob);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyReservoirSampling>
		void process_(MyReservoirSampling const &rs, const hm4::Pair *pair, Result<Protocol> &result, OutputBlob &blob){
			auto &container = blob.container();

			if (pair == nullptr)
				return result.set_container(container);

			using List = typename MyReservoirSampling::List;

			const auto *rs_data = reinterpret_cast<const List *>(pair->getValC());

			for(size_t i = 0; i < rs.size(); ++i)
				if (auto const &x = rs_data->items[i]; x)
					container.push_back(x.getItem());

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rsget",	"RSGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct RSGETCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// RSGETCOUNT key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace rs_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint16_t	>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyReservoirSampling = T;

					MyReservoirSampling const rs{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, rs.bytes());

					return process_(rs, pair, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyReservoirSampling>
		void process_(MyReservoirSampling const &, const hm4::Pair *pair, Result<Protocol> &result){
			if (pair == nullptr)
				return result.set_0();

			using List = typename MyReservoirSampling::List;

			const auto *rs_data = reinterpret_cast<const List *>(pair->getValC());

			return result.set(rs_data->getCount());
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"rsgetcount",	"RSGETCOUNT"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hh";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				RSADD		,
				RSRESERVE	,
				RSGET		,
				RSGETCOUNT
			>(pack);
		}
	};



} // namespace

