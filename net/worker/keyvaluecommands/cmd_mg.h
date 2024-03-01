#include "base.h"
#include "misragries.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

namespace net::worker::commands::MG{
	namespace mg_impl_{
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
			using namespace misra_gries;

			switch(t){
			case  16 : return f(type_identity<RawMisraGries16	>{});
			case  32 : return f(type_identity<RawMisraGries32	>{});
			case  40 : return f(type_identity<RawMisraGries40	>{});
			case  64 : return f(type_identity<RawMisraGries64	>{});
			case 128 : return f(type_identity<RawMisraGries128	>{});
			case 256 : return f(type_identity<RawMisraGries256	>{});
			default  : return f(type_identity<std::nullptr_t	>{});
			}
		}
	} // namespace mg_impl_



	template<class Protocol, class DBAdapter>
	struct MGADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MGADD key slots bytes item item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace mg_impl_;

		//	auto const varg = 4;

			if (p.size() < 5)
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
					using MyRawMisraGries = T;

					MyRawMisraGries const mg{ slots };

					return process_(mg, key, p, *db, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<typename MyRawMisraGries>
		void process_(MyRawMisraGries &mg, std::string_view const key, ParamContainer const &p, typename DBAdapter::List &list, Result<Protocol> &result) const{
			auto const varg = 4;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &item = *itk; !mg.isItemValid(item))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(list, key, mg.bytes());

			using MyMGADDFactory = MGADDFactory<ParamContainer::iterator, MyRawMisraGries>;

			MyMGADDFactory factory{ key, pair, mg, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, list, factory);

			return result.set(
				factory.getResult()
			);
		}

	private:
		template<typename It, typename MyRawMisraGries>
		struct MGADDFactory : hm4::PairFactory::IFactoryAction<1,1,MGADDFactory<It, MyRawMisraGries> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,MGADDFactory>;

			MGADDFactory(std::string_view const key, const Pair *pair, MyRawMisraGries mg, It begin, It end) :
							Base::IFactoryAction	(key, mg.bytes(), pair),
							mg			(mg	),
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
				using Item = typename MyRawMisraGries::Item;

				Item *mg_data = reinterpret_cast<Item *>(pair->getValC());

				bool result = false;

				for(auto itk = begin; itk != end; ++itk){
					auto const &item = *itk;

					result |= mg.add(mg_data, item);
				}

				return result;
			}

		private:
			MyRawMisraGries		mg;
			It			begin;
			It			end;
			bool			result = false;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mgadd",	"MGADD"	,
			"mgincr",	"MGINCR"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MGADDGET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MGADDGET key slots bytes item

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace mg_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			auto const &key  = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const slots = from_string<uint8_t>(p[2]);

			if (!isSlotsValid(slots))
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const bytes = from_string<size_t	>(p[3]);

			auto const &item = p[4];

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					using MyRawMisraGries = T;

					MyRawMisraGries const mg{ slots };

					return process_(mg, key, item, *db, result);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<typename MyRawMisraGries>
		void process_(MyRawMisraGries &mg, std::string_view const key, std::string_view const item, typename DBAdapter::List &list, Result<Protocol> &result) const{
			if (!mg.isItemValid(item))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(list, key, mg.bytes());

			using MyMGADDFactory = MGADDGETFactory<MyRawMisraGries>;

			MyMGADDFactory factory{ key, pair, mg, item };

			insertHintVFactory(pair, list, factory);

			return result.set(
				factory.getScore()
			);
		}

	private:
		template<typename MyRawMisraGries>
		struct MGADDGETFactory : hm4::PairFactory::IFactoryAction<1,1,MGADDGETFactory<MyRawMisraGries> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,MGADDGETFactory>;

			MGADDGETFactory(std::string_view const key, const Pair *pair, MyRawMisraGries mg, std::string_view const item) :
							Base::IFactoryAction	(key, mg.bytes(), pair),
							mg			(mg	),
							item			(item	){}

			void action(Pair *pair){
				this->score = action_(pair);
			}

			constexpr uint64_t getScore() const{
				return score;
			}

		private:
			uint64_t action_(Pair *pair) const{
				using Item = typename MyRawMisraGries::Item;

				Item *mg_data = reinterpret_cast<Item *>(pair->getValC());

				return mg.add(mg_data, item);
			}

		private:
			MyRawMisraGries		mg;
			std::string_view	item;
			uint64_t		score = 0;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mgaddget",	"MGADDGET"
		};
	};




	template<class Protocol, class DBAdapter>
	struct MGRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MGRESERVE key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace mg_impl_;

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
					using MyRawMisraGries = T;

					MyRawMisraGries const mg{ slots };

					hm4::insertV<hm4::PairFactory::Reserve>(*db, key, mg.bytes());

					return result.set_1();
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mgreserve",	"MGRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MGGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MGGET key slots bytes

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace mg_impl_;

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
					using MyRawMisraGries = T;

					MyRawMisraGries const mg{ slots };

					const auto *pair = hm4::getPairPtrWithSize(*db, key, mg.bytes());

					return process_(mg, pair, result, blob);
				}
			};

			return type_dispatch(bytes, f);
		}

	private:
		template<class MyRawMisraGries>
		void process_(MyRawMisraGries const &mg, const hm4::Pair *pair, Result<Protocol> &result, OutputBlob &blob){
			auto &container = blob.container();

			if (pair == nullptr)
				return result.set_container(container);

			auto &bcontainer = blob.bcontainer();

			using Item = typename MyRawMisraGries::Item;

			const auto *mg_data = reinterpret_cast<const Item *>(pair->getValC());

			for(size_t i = 0; i < mg.size(); ++i){
				auto const &x = mg_data[i];

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
			"mgget",	"MGGET"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mg";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MGADD		,
				MGADDGET	,
				MGRESERVE	,
				MGGET
			>(pack);
		}
	};



} // namespace

