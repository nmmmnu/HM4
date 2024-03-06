#include "base.h"
#include "murmur_hash_64a.h"
#include "pair_vfactory.h"
#include "matrix.h"

#include <algorithm>
#include <limits>
#include <type_traits>

namespace net::worker::commands::CMS{
	namespace cms_impl_{
		namespace{

			using Pair = hm4::Pair;

			constexpr auto MAX_SIZE = hm4::PairConf::MAX_VAL_SIZE;



			template<typename T>
			T incr(T &a, uint64_t increment){
				constexpr auto MAX    = std::numeric_limits<T>::max();
				constexpr auto MAX_BE = htobe(MAX);

				auto const x = betoh<T>(a);

				if (increment > MAX || x > MAX - increment){
					// return max
					a = MAX_BE;

					return MAX;
				}else{
					// cast is safe now
					T const c = x + static_cast<T>(increment);

					a = htobe<T>(c);

					return c;
				}
			}



			template<typename T>
			void cms_add(Matrix<T> cms, char *data, std::string_view item, uint64_t const n){
				for(size_t i = 0; i < cms.getY(); ++i)
					incr( cms(data, murmur_hash64a(item, i) % cms.getX(), i), n);
			}

			template<typename T>
			uint64_t cms_add_count(Matrix<T> cms, char *data, std::string_view item, uint64_t const n){
				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < cms.getY(); ++i){
					auto const x = incr( cms(data, murmur_hash64a(item, i) % cms.getX(), i), n);

					count = std::min<uint64_t>(count, x);
				}

				return count;
			}

			template<typename T>
			uint64_t cms_count(Matrix<T> cms, const char *data, std::string_view item){
				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < cms.getY(); ++i){
					auto const x = cms(data, murmur_hash64a(item, i) % cms.getX(), i);

					count = std::min<uint64_t>(count, betoh<T>(x));
				}

				return count;
			}

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

		} // namespace
	} // namespace cms_impl_



	template<class Protocol, class DBAdapter>
	struct CMSADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cms_impl_;

			auto const varg = 5;

			if (p.size() < 7 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = from_string<uint64_t>(p[2]);
			auto const d = from_string<uint64_t>(p[3]);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return process_(key, p, Matrix<T>(w, d), *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, ParamContainer const &p, Matrix<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cms_impl_;

			if (cms.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			auto const varg = 5;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			using MyCMSADD_Factory = CMSADD_Factory<T, ParamContainer::iterator>;

			if (pair && hm4::canInsertHintValSize(list, pair, cms.bytes()))
				hm4::proceedInsertHintV<MyCMSADD_Factory>(list, pair, key, pair, cms, std::begin(p) + varg, std::end(p));
			else
				hm4::insertV<MyCMSADD_Factory>(list, key, pair, cms, std::begin(p) + varg, std::end(p));

			return result.set();
		}

	private:
		template<typename T, typename It>
		struct CMSADD_Factory : hm4::PairFactory::IFactoryAction<1,1, CMSADD_Factory<T, It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, CMSADD_Factory<T, It> >;

			constexpr CMSADD_Factory(std::string_view const key, const Pair *pair, Matrix<T> cms, It begin, It end) :
							Base::IFactoryAction	(key, cms.bytes(), pair),
							cms			(cms	),
							begin			(begin	),
							end			(end	){}

			void action(Pair *pair) const{
				char *data = pair->getValC();

				for(auto itk = begin; itk != end; itk += 2){
					auto const &val = *itk;
					auto const n   = std::max<uint64_t>( from_string<uint64_t>( *std::next(itk) ), 1);

					using namespace cms_impl_;

					cms_add(cms, data, val, n);
				}
			}

		private:
			Matrix<T>	cms;
			It		begin;
			It		end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmsadd",	"CMSADD",
			"cmsincr",	"CMSINCR",
			"cmsincrby",	"CMSINCRBY"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CMSADDCOUNT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cms_impl_;

			if (p.size() != 7)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = from_string<uint64_t>(p[2]);
			auto const d = from_string<uint64_t>(p[3]);
			auto const t = from_string<uint8_t>(p[4]);

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
					return process_(key, item, itemCount, Matrix<T>(w, d), *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, std::string_view const item, T const itemCount, Matrix<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cms_impl_;

			if (cms.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			using MyCMSADD_Factory = CMSADDCOUNT_Factory<T>;

			MyCMSADD_Factory factory{ key, pair, cms, item, itemCount };

			insertHintVFactory(pair, list, factory);

			return result.set(
				factory.getScore()
			);
		}

	private:
		template<typename T>
		struct CMSADDCOUNT_Factory : hm4::PairFactory::IFactoryAction<1,1, CMSADDCOUNT_Factory<T> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1, CMSADDCOUNT_Factory<T> >;

			constexpr CMSADDCOUNT_Factory(std::string_view const key, const Pair *pair, Matrix<T> cms, std::string_view const item, T const itemCount) :
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

				using namespace cms_impl_;

				cms_add(cms, data, item, itemCount);

				return cms_count(cms, data, item);
			}

		private:
			Matrix<T>		cms;
			std::string_view	item;
			T			itemCount;
			uint64_t		score = 0;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmsaddcount",	"CMSADDCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CMSRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cms_impl_;

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
					return process_(key, Matrix<T>(w, d), *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, Matrix<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cms_impl_;

			if (cms.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			hm4::insertV<hm4::PairFactory::Reserve>(list, key, cms.bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmsreserve",	"CMSRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CMSCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cms_impl_;

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
					return process_(key, val, Matrix<T>(w, d), *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, std::string_view item, Matrix<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cms_impl_;

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			if (! pair)
				return result.set_0();

			auto const count = cms_count(cms, pair->getValC(), item);

			return result.set( count );
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmscount",	"CMSCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CMSMCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace cms_impl_;

			if (p.size() < 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

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
					return process_(key, p, Matrix<T>(w, d), *db, result, blob);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, ParamContainer const &p, Matrix<T> const cms, typename DBAdapter::List &list, Result<Protocol> &result, OutputBlob &blob) const{
			using namespace cms_impl_;

			auto const varg = 5;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(list, key, cms.bytes());

			auto &container  = blob.container();

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			if (! pair){
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					container.push_back("0");
				}
			}else{
				auto &bcontainer = blob.bcontainer();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const &item = *itk;

					auto const n = cms_count(cms, pair->getValC(), item);

					bcontainer.push_back();

					container.push_back( to_string(n, bcontainer.back()) );
				}
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmsmcount",	"CMSMCOUNT",
			"cmsquery",	"CMSQUERY",
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cms";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CMSADD		,
				CMSADDCOUNT	,
				CMSRESERVE	,
				CMSCOUNT	,
				CMSMCOUNT
			>(pack);
		}
	};



} // namespace



