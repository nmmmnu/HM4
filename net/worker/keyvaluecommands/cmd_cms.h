#include "base.h"
#include "murmur_hash_64a.h"
#include "pair_vfactory.h"
#include "shared_incr.h"
#include "shared_hint.h"
#include "matrix.h"

#include <algorithm>
#include <type_traits>

namespace net::worker::commands::CMS{
	namespace impl_{

		using Pair = hm4::Pair;

		constexpr auto MAX_SIZE = hm4::PairConf::MAX_VAL_SIZE;



		template<typename T>
		struct CMS{
			constexpr CMS(uint64_t w, uint64_t d) : m(w, d){}

			constexpr auto bytes() const{
				return m.bytes();
			}

			template<bool Sign>
			void add(char *data, std::string_view item, uint64_t const n) const{
				for(size_t i = 0; i < m.getY(); ++i){
					using namespace shared::incr;
					incr<Sign>( m(data, murmur_hash64a(item, i) % m.getX(), i), n);
				}
			}

			template<bool Sign>
			uint64_t add_count(char *data, std::string_view item, uint64_t const n) const{
				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < m.getY(); ++i){
					using namespace shared::incr;
					auto const x = incr<Sign>( m(data, murmur_hash64a(item, i) % m.getX(), i), n);

					count = std::min<uint64_t>(count, x);
				}

				return count;
			}

			uint64_t count(const char *data, std::string_view item) const{
				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < m.getY(); ++i){
					auto const x = m(data, murmur_hash64a(item, i) % m.getX(), i);

					count = std::min<uint64_t>(count, betoh<T>(x));
				}

				return count;
			}

		private:
			Matrix<T> m;
		};



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
			case 64 : return f(type_identity<uint64_t	>{});
			default : return f(type_identity<std::nullptr_t	>{});
			}
		}


		template<class Protocol, class DBAdapter, bool Sign>
		struct CMSMutate{
			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
				auto const varg = 5;

				if (p.size() < 7 || (p.size() - varg) % 2 != 0)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const d = from_string<uint64_t	>(p[3]);
				auto const t = from_string<uint8_t	>(p[4]);

				if (w == 0 || d == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else{
						return process_(key, p, CMS<T>{ w, d }, *db, result);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			void process_(std::string_view key, ParamContainer const &p, impl_::CMS<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
				if (cms.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				auto const varg = 5;

				for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
					if (const auto &val = *itk; val.empty())
						return result.set_error(ResultErrorMessages::EMPTY_VAL);

				const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

				using MyCMSADD_Factory = CMSADD_Factory<T>;

				MyCMSADD_Factory factory{ key, pair, cms, std::begin(p) + varg, std::end(p) };

				insertHintVFactory(list, pair, factory);

				return result.set();
			}

		private:
			template<typename T>
			struct CMSADD_Factory : hm4::PairFactory::IFactoryAction<1,1, CMSADD_Factory<T> >{
				using Pair = hm4::Pair;
				using Base = hm4::PairFactory::IFactoryAction<1,1, CMSADD_Factory<T> >;

				using It   = ParamContainer::const_iterator;
				using CMST = impl_::CMS<T>;

				constexpr CMSADD_Factory(std::string_view const key, const Pair *pair, CMST cms, It begin, It end) :
								Base::IFactoryAction	(key, cms.bytes(), pair),
								cms			(cms	),
								begin			(begin	),
								end			(end	){}

				void action(Pair *pair) const{
					char *data = pair->getValC();

					for(auto itk = begin; itk != end; itk += 2){
						auto const &val = *itk;
						auto const n    = std::max<uint64_t>( from_string<uint64_t>( *std::next(itk) ), 1);

						cms. template add<Sign>(data, val, n);
					}
				}

			private:
				CMST	cms;
				It	begin;
				It	end;
			};
		};



		template<class Protocol, class DBAdapter, bool Sign>
		struct CMSMutateCount{
			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
				if (p.size() != 7)
					return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const d = from_string<uint64_t	>(p[3]);
				auto const t = from_string<uint8_t	>(p[4]);

				const auto &item = p[5];

				if (w == 0 || d == 0 || item.empty())
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else if (auto const itemCount = from_string<T>(p[6]); itemCount == 0){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
					}else{
						return process_(key, item, itemCount, CMS<T>{ w, d }, *db, result);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			void process_(std::string_view key, std::string_view const item, T const itemCount, impl_::CMS<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
				if (cms.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

				using MyCMSADD_Factory = CMSADDCOUNT_Factory<T>;

				MyCMSADD_Factory factory{ key, pair, cms, item, itemCount };

				insertHintVFactory(list, pair, factory);

				return result.set(
					factory.getScore()
				);
			}

		private:
			template<typename T>
			struct CMSADDCOUNT_Factory : hm4::PairFactory::IFactoryAction<1,1, CMSADDCOUNT_Factory<T> >{
				using Pair = hm4::Pair;
				using Base = hm4::PairFactory::IFactoryAction<1,1, CMSADDCOUNT_Factory<T> >;

				using CMST = impl_::CMS<T>;

				constexpr CMSADDCOUNT_Factory(std::string_view const key, const Pair *pair, CMST cms, std::string_view const item, T const itemCount) :
								Base::IFactoryAction	(key, cms.bytes(), pair),
								cms			(cms		),
								item			(item		),
								itemCount		(itemCount	){}

				void action(Pair *pair){
					this->score = action_(pair);
				}

				constexpr uint64_t getScore() const{
					return score;
				}

			private:
				uint64_t action_(Pair *pair) const{
					char *data = pair->getValC();

					return cms. template add_count<Sign>(data, item, itemCount);
				}

			private:
				CMST			cms;
				std::string_view	item;
				T			itemCount;
				uint64_t		score = 0;
			};
		};

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct CMSADD : BaseCommandRW<Protocol,DBAdapter>{

		CMSADD() : BaseCommandRW<Protocol,DBAdapter>("CMSADD", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			CMSMutate<Protocol, DBAdapter, true> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsadd",	"CMSADD",
			"cmsincr",	"CMSINCR",
			"cmsincrby",	"CMSINCRBY"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSREM : BaseCommandRW<Protocol,DBAdapter>{

		CMSREM() : BaseCommandRW<Protocol,DBAdapter>("CMSREM", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			CMSMutate<Protocol, DBAdapter, false> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsrem",	"CMSREM",
			"cmsdecr",	"CMSDECR",
			"cmsdecrby",	"CMSDECRBY"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSADDCOUNT : BaseCommandRW<Protocol,DBAdapter>{

		CMSADDCOUNT() : BaseCommandRW<Protocol,DBAdapter>("CMSADDCOUNT", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			CMSMutateCount<Protocol, DBAdapter, true> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsaddcount",	"CMSADDCOUNT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSREMCOUNT : BaseCommandRW<Protocol,DBAdapter>{

		CMSREMCOUNT() : BaseCommandRW<Protocol,DBAdapter>("CMSREMCOUNT", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace impl_;

			CMSMutateCount<Protocol, DBAdapter, false> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsremcount",	"CMSREMCOUNT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSRESERVE : BaseCommandRW<Protocol,DBAdapter>{

		CMSRESERVE() : BaseCommandRW<Protocol,DBAdapter>("CMSRESERVE", std::begin(cmd__), std::end(cmd__)){}


		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = std::max<uint64_t>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return processT__(key, CMS<T>{ w, d }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}


		template<typename T>
		static void processT__(std::string_view key, impl_::CMS<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result){
			using namespace impl_;

			if (cms.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			hm4::insertV<hm4::PairFactory::Reserve>(list, key, cms.bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsreserve",	"CMSRESERVE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		CMSCOUNT() : BaseCommandRO<Protocol,DBAdapter>("CMSCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = std::max<uint64_t>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto &val = p[5];

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return processT__(key, val, CMS<T>{ w, d }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}


		template<typename T>
		static void processT__(std::string_view key, std::string_view item, impl_::CMS<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result){
			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			if (! pair)
				return result.set_0();

			auto const count = cms.count(pair->getValC(), item);

			return result.set( count );
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmscount",	"CMSCOUNT"
		};

	};



	template<class Protocol, class DBAdapter>
	struct CMSMCOUNT : BaseCommandRO<Protocol,DBAdapter>{

		CMSMCOUNT() : BaseCommandRO<Protocol,DBAdapter>("CMSMCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			using namespace impl_;

			if (p.size() < 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = std::max<uint64_t	>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t	>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t	>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return processT__(key, p, CMS<T>{ w, d }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

		template<typename T>
		static void processT__(std::string_view key, ParamContainer const &p, impl_::CMS<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result){
			using namespace impl_;

			auto const varg = 5;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			Container container;

			if (! pair){
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					container.push_back("0");
				}
			}else{
				BContainer bcontainer;

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const &item = *itk;

					auto const n = cms.count(pair->getValC(), item);

					bcontainer.push_back();

					container.push_back( to_string(n, bcontainer.back()) );
				}
			}

			return result.set_container(container);
		}

	private:
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"cmsmcount",	"CMSMCOUNT"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cms";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CMSADD		,
				CMSREM		,
				CMSADDCOUNT	,
				CMSREMCOUNT	,
				CMSRESERVE	,
				CMSCOUNT	,
				CMSMCOUNT
			>(pack);
		}
	};



} // namespace



