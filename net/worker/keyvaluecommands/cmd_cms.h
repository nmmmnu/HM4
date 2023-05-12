#include "base.h"
#include "murmur_hash_64a.h"
#include "pair_vfactory.h"
#include "matrix.h"

#include <algorithm>
#include <limits>
#include <type_traits>

namespace net::worker::commands::CMS{

	namespace cms_impl_{
		using Pair = hm4::Pair;

		constexpr auto MAX_SIZE = hm4::PairConf::MAX_VAL_SIZE;



		template<typename T>
		void incr(T &a, uint64_t increment){
			constexpr auto MAX = std::numeric_limits<T>::max();

			auto const x = betoh<T>(a);

			if (increment > MAX || x > MAX - increment){
				// return max
				// no need htobe
				a = MAX;

				return;
			}else{
				// cast is safe now.
				a = htobe<T>(
					x +
					static_cast<T>(increment)
				);

				return;
			}
		}



		template<typename T>
		void cms_add(Matrix<T> cms, char *data, std::string_view item, uint64_t const n){
			for(size_t i = 0; i < cms.getY(); ++i)
				incr( cms(data, murmur_hash64a(item, i) % cms.getX(), i), n);
		}

		template<typename T>
		uint64_t cms_count(Matrix<T> cms, const char *data, std::string_view item){
			uint64_t count = std::numeric_limits<uint64_t>::max();

			for(size_t i = 0; i < cms.getY(); ++i){
				auto const &c = cms(data, murmur_hash64a(item, i) % cms.getX(), i);

				count = std::min<uint64_t>(count, betoh<T>(c));
			}

			return count;
		}

		template<typename T>
		struct type_identity{
			// C++20 std::type_identity
			using type = T;
		};

		template<typename F, typename... Args>
		auto type_dispatch(uint8_t const t, F f, Args&&... args){
			switch(t){
			case  8 : return f(type_identity<uint8_t	>{});
			case 16 : return f(type_identity<uint16_t	>{});
			case 32 : return f(type_identity<uint32_t	>{});
			case 64 : return f(type_identity<uint64_t	>{});
			default : return f(type_identity<std::nullptr_t	>{});
			}
		}
	}



	template<class Protocol, class DBAdapter>
	struct CMSADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cms_impl_;

			if (p.size() < 7 || p.size() % 2 == 0)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const w = from_string<uint64_t>(p[2]);
			auto const d = from_string<uint64_t>(p[3]);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return;

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
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
				return;
			}

			auto const varg = 5;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &val = *itk; val.empty())
					return;

			const auto *pair = hm4::getPair_(list, key, [max_size = cms.bytes()](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == max_size)
					return & *it;
				else
					return nullptr;
			});

			using MyCMSADD_Factory = CMSADD_Factory<T, ParamContainer::iterator>;

			if (pair && hm4::canInsertHint(list, pair, cms.bytes()))
				hm4::proceedInsertHintV<MyCMSADD_Factory>(list, pair, key, pair, cms, std::begin(p) + varg, std::end(p));
			else
				hm4::insertV<MyCMSADD_Factory>(list, key, pair, cms, std::begin(p) + varg, std::end(p));

			return result.set();
		}

	private:
		template<typename T, typename It>
		struct CMSADD_Factory : hm4::PairFactory::IFactoryAction<1,1>{
			using Pair = hm4::Pair;

			CMSADD_Factory(std::string_view const key, const Pair *pair, Matrix<T> cms, It begin, It end) :
							IFactoryAction	(key, cms.bytes(), pair),
							cms		(cms	),
							begin		(begin	),
							end		(end	){}
		private:
			void action(Pair *pair) override{
				char *data = pair->getValC();

				for(auto itk = begin; itk != end; itk += 2){
					auto const &val = *itk;
					auto const n   = std::max<uint64_t>( from_string<uint64_t>( *std::next(itk) ), 1);

					using namespace cms_impl_;

					cms_add(cms, data, val, n);
				}
			}

		private:
			Matrix<T>		cms;
			It			begin;
			It			end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cmsadd",	"CMSADD",
			"cmsincr",	"CMSINCR",
			"cmsincrby",	"CMSINCRBY"
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
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const w = std::max<uint64_t>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return;

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
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
				return;
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
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const w = std::max<uint64_t>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return;

			const auto &val = p[5];

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
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

			const auto *pair = hm4::getPair_(list, key, [max_size = cms.bytes()](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == max_size)
					return & *it;
				else
					return nullptr;
			});

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
				return;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const w = std::max<uint64_t>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t>(p[4]);

			if (w == 0 || d == 0)
				return;

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
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

			const auto *pair = hm4::getPair_(list, key, [max_size = cms.bytes()](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == max_size)
					return & *it;
				else
					return nullptr;
			});

			if (! pair)
				return result.set_0();

			auto &container  = blob.container;
			auto &bcontainer = blob.bcontainer;

			container.clear();
			bcontainer.clear();

			auto const varg = 5;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &item = *itk;

				auto const n = cms_count(cms, pair->getValC(), item);

				bcontainer.push_back();

				container.push_back( to_string(n, bcontainer.back()) );
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
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CMSADD		,
				CMSRESERVE	,
				CMSCOUNT	,
				CMSMCOUNT
			>(pack);
		}
	};



} // namespace



#if 0
	constexpr static size_t calcW(size_t number_of_elements){
		// "standard" calculation
		//	w = ceil(exp(1) / epsilon)
		//	d = ceil(log(1 / delta))
		//
		// redis calculation
		//	w = ceil(2 / epsilon)
		//	d = ceil(log10(delta) / log10(0.5))

		// avoiding math
		double const e_c = 2.71828 * 0.1;

		return static_cast<size_t>(
				e_c * static_cast<double>(number_of_elements)
		);
	}

	constexpr static size_t calcD(double){
		// 5 always good, no, really :)
		return 5;
	}

	static_assert(
		std::is_same_v<T,  uint8_t> ||
		std::is_same_v<T, uint16_t> ||
		std::is_same_v<T, uint32_t> ||
		std::is_same_v<T, uint64_t>, "Please check counter type"
	);
#endif

