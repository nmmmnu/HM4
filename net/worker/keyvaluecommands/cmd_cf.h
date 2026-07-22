#include "base.h"
#include "pair_vfactory.h"
#include "shared_incr.h"

#include "cuckoofilter.h"

//#include <algorithm>
#include <type_traits>

namespace net::worker::commands::CF{
	namespace impl_{

		using namespace cuckoo_filter;

		using Pair = hm4::Pair;

		constexpr auto MAX_SIZE = hm4::PairConf::MAX_VAL_SIZE;



		template<typename T>
		struct type_identity{
			// C++20 std::type_identity
			using type = T;
		};

		template<typename F>
		auto type_dispatch(uint8_t const t, F f){
			switch(t){
			case  8 : return f(type_identity<uint8_t	>{});
			case 16 : return f(type_identity<uint16_t	>{});
			case 32 : return f(type_identity<uint32_t	>{});
			default : return f(type_identity<std::nullptr_t	>{});
			}
		}



		template<class Protocol, class DBAdapter, bool Add, bool MultiValues>
		struct CFMutate{

			// CFADD key width integer_size value
			// CFREM key width integer_size value

			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
				auto const varg = 4;

				if constexpr(MultiValues){
					if (p.size()  < 5)
						return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

				}else{
					if (p.size() != 5)
						return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);
				}

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const t = from_string<uint8_t	>(p[3]);

				if (w == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					if (const auto &val = *itk; val.empty())
						return result.set_error(ResultErrorMessages::EMPTY_VAL);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else{
						return process__(key, p, CuckooFilter<T>{ w }, *db, result, blob);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			static void process__(std::string_view key, ParamContainer const &p, CuckooFilter<T> cf, typename DBAdapter::List &list, Result<Protocol> &result, OutputBlob &blob){
				if (cf.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				auto const varg = 4;

				auto &container = blob.construct<OutputBlob::Container>();

				const auto *pair = hm4::getPairPtrWithSize(list, key, cf.bytes());

				using MyCFADD_Factory = CFADD_Factory<T>;

				MyCFADD_Factory factory{ key, pair, cf, std::begin(p) + varg, std::end(p), container };

				insertHintVFactory(list, pair, factory);

				if constexpr(MultiValues){
					// container
					return result.set_container(
						container
					);
				}else{
					// bool
					return result.set_number_sv(
						container[0]
					);
				}
			}

		private:
			template<typename T>
			struct CFADD_Factory : hm4::PairFactory::IFactoryAction<1,1, CFADD_Factory<T> >{
				using Pair = hm4::Pair;
				using Base = hm4::PairFactory::IFactoryAction<1,1, CFADD_Factory<T> >;

				using It     = ParamContainer::const_iterator;
				using CFT    = CuckooFilter<T>;

				constexpr CFADD_Factory(std::string_view const key, const Pair *pair, CFT cf, It begin, It end, OutputBlob::Container &container) :
								Base::IFactoryAction	(key, cf.bytes(), pair),
								cf			(cf		),
								begin			(begin		),
								end			(end		),
								container		(container	){}

				void action(Pair *pair){
					auto *data = hm4::getValAs<T>(pair);

					for(auto itk = begin; itk != end; ++itk){
						auto const &val = *itk;

						if constexpr(Add){
							container.push_back(
								cf.insert(data, val) ? "1" : "0"
							);
						}else{
							container.push_back(
								cf.remove(data, val) ? "1" : "0"
							);
						}
					}
				}

			private:
				CFT			cf;
				It			begin;
				It			end;
				OutputBlob::Container	&container;
			};
		};



		template<class Protocol, class DBAdapter, bool MultiValues>
		struct CFExists{

			// CFEXISTS key width integer_size value

			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
				auto const varg = 4;

				if constexpr(MultiValues){
					if (p.size()  < 5)
						return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

				}else{
					if (p.size() != 5)
						return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);
				}

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const t = from_string<uint8_t	>(p[3]);

				if (w == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					if (const auto &val = *itk; val.empty())
						return result.set_error(ResultErrorMessages::EMPTY_VAL);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else{
						return process__(key, p, CuckooFilter<T>{ w }, *db, result, blob);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			static void process__(std::string_view key, ParamContainer const &p, CuckooFilter<T> cf, typename DBAdapter::List &list, Result<Protocol> &result, OutputBlob &blob){
				if (cf.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				auto const varg = 4;

				auto &container = blob.construct<OutputBlob::Container>();

				const auto *pair = hm4::getPairPtrWithSize(list, key, cf.bytes());

				if (!pair){
					if constexpr(MultiValues){
						for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
							container.push_back("0");

						// container
						return result.set_container(
							container
						);
					}else{
						// bool
						return result.set_0();
					}
				}

				auto *data = hm4::getValAs<T>(pair);

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const &val = *itk;

					container.push_back(
						cf.lookup(data, val) ? "1" : "0"
					);
				}

				if constexpr(MultiValues){
					// container
					return result.set_container(
						container
					);
				}else{
					// bool
					return result.set_number_sv(
						container[0]
					);
				}
			}
		};

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct CFADD : BaseCommandRW<Protocol,DBAdapter>{

		CFADD() : BaseCommandRW<Protocol,DBAdapter>("CFADD", std::begin(cmd__), std::end(cmd__)){}

		// CFADD key width integer_size value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFMutate<Protocol, DBAdapter, 1, 0> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfadd",	"CFADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFMADD : BaseCommandRW<Protocol,DBAdapter>{

		CFMADD() : BaseCommandRW<Protocol,DBAdapter>("CFMADD", std::begin(cmd__), std::end(cmd__)){}

		// CFMADD key width integer_size value value...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFMutate<Protocol, DBAdapter, 1, 1> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfmadd",	"CFMADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFREM : BaseCommandRW<Protocol,DBAdapter>{

		CFREM() : BaseCommandRW<Protocol,DBAdapter>("CFREM", std::begin(cmd__), std::end(cmd__)){}

		// CFREM key width integer_size value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFMutate<Protocol, DBAdapter, 0, 0> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfrem",	"CFREM"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFMREM : BaseCommandRW<Protocol,DBAdapter>{

		CFMREM() : BaseCommandRW<Protocol,DBAdapter>("CFMREM", std::begin(cmd__), std::end(cmd__)){}

		// CFMREM key width integer_size value value...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFMutate<Protocol, DBAdapter, 0, 1> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfmrem",	"CFMREM"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFEXISTS : BaseCommandRW<Protocol,DBAdapter>{

		CFEXISTS() : BaseCommandRW<Protocol,DBAdapter>("CFEXISTS", std::begin(cmd__), std::end(cmd__)){}

		// CFEXISTS key width integer_size value

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFExists<Protocol, DBAdapter, false> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfexists",	"CFEXISTS"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFMEXISTS : BaseCommandRW<Protocol,DBAdapter>{

		CFMEXISTS() : BaseCommandRW<Protocol,DBAdapter>("CFMEXISTS", std::begin(cmd__), std::end(cmd__)){}

		// CFMEXISTS key width integer_size value...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			CFExists<Protocol, DBAdapter, true> mut;

			return mut(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfmexists",	"CFMEXISTS"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CFRESERVE : BaseCommandRW<Protocol,DBAdapter>{

		CFRESERVE() : BaseCommandRW<Protocol,DBAdapter>("CFRESERVE", std::begin(cmd__), std::end(cmd__)){}

		// CFRESERVE key width integer_size

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = from_string<uint64_t	>(p[2]);
			auto const t = from_string<uint8_t	>(p[3]);

			if (w == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return processT__(key, CuckooFilter<T>{ w }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

		template<typename T>
		static void processT__(std::string_view key, impl_::CuckooFilter<T> cf, typename DBAdapter::List &list, Result<Protocol> &result){
			using namespace impl_;

			if (cf.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			hm4::insertV<hm4::PairFactory::Reserve>(list, key, cf.bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cfreserve",	"CFRESERVE"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cf";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CFRESERVE	,
				CFADD		,	CFMADD		,
				CFREM		,	CFMREM		,
				CFEXISTS	,	CFMEXISTS
			>(pack);
		}
	};



} // namespace



