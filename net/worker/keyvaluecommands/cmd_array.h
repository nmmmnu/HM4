#include "base.h"
#include "logger.h"

#include "shared_bitops.h"

namespace net::worker::commands::CV{
	namespace cv_impl_{
		using Pair = hm4::Pair;

		using size_type				= uint64_t;

		template<typename T>
		constexpr size_type CV_MAX		= hm4::PairConf::MAX_VAL_SIZE / sizeof(T);

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

		template<typename T>
		constexpr T *cast_cv(void *p){
			return reinterpret_cast<T *>(p);
		}

		template<typename T>
		constexpr const T *cast_cv(const void *p){
			return reinterpret_cast<const T *>(p);
		}

		template<typename T, typename It>
		struct CV_Factory : hm4::PairFactory::IFactory{
			CV_Factory(std::string_view const key, uint64_t val_size, It begin, It end, const Pair *pair) :
							key		(key		),
							val_size	(val_size	),
							begin		(begin		),
							end		(end		),
							old_pair	(pair		){}

			constexpr std::string_view getKey() const final{
				return key;
			}

			constexpr uint32_t getCreated() const final{
				return 0;
			}

			constexpr size_t bytes() const final{
				return Pair::bytes(key.size(), val_size);
			}

			void createHint(Pair *pair) final{
				add_(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0,1,1>(pair, key, val_size, 0, 0);
				create_(pair);

				add_(pair);
			}

		private:
			void create_(Pair *pair) const{
				char *data = pair->getValC();

				if (old_pair){
					memcpy(data, old_pair->getValC(), val_size);
				}else{
					memset(data, '\0', val_size);
				}
			}

			void add_(Pair *pair) const{
				char *data = pair->getValC();

				for(auto it = begin; it != end; it += 2){
					auto const n = from_string<size_type>(*it);

					if (n >= CV_MAX<T>)
						continue;

					T const value = from_string<uint64_t>(*std::next(it));

					auto *cv = cast_cv<T>(data);

					cv[n] = htobe<T>(value);
				}
			}

		private:
			std::string_view	key;
			uint64_t		val_size;
			It			begin;
			It			end;
			const Pair		*old_pair;
		};
	}

/*
	template<class Protocol, class DBAdapter>
	struct CVSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be even number arguments
			// cvset key 32 0 5
			if (p.size() < 5 || p.size() % 2 == 1)
				return;

			using namespace cv_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const [ok, n_size] = getNSize_(p);

			if (!ok)
				return;

			const auto *pair = hm4::getPairPtr(*db, key);

			using MyBITSET_Factory = BITSET_Factory<ParamContainer::iterator>;

			auto const varg = 2;

			if (pair && hm4::canInsertHint(*db, pair, n_size))
				hm4::proceedInsertHintV<MyBITSET_Factory>(*db, pair, key, n_size, std::begin(p) + varg, std::end(p), pair);
			else
				hm4::insertV<MyBITSET_Factory>(*db, key, n_size, std::begin(p) + varg, std::end(p), pair);

			return result.set();
		}

	private:
		static auto getNSize_(ParamContainer const &p){
			using namespace bit_impl_;

			bool ok = false;
			size_type max = 0;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n < BIT_MAX){
					ok  = true;
					max = std::max(max, n);
				}
			}

			return std::make_pair(ok, BitOps::size(max));
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvset",	"CVSET"	,
			"cvmset",	"CVMSET"
		};
	};
*/


	template<class Protocol, class DBAdapter>
	struct CVGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// cvget key 32 5
			if (p.size() != 4)
				return;

			using namespace cv_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			auto const t		= from_string<uint8_t>(p[2]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
				}else{
					return process_<T>(key, p, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, ParamContainer const &p, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cv_impl_;

			const auto n		= from_string<size_type>(p[3]);

			const auto *data = hm4::getPair_(list, key, [n](bool b, auto it) -> const char *{
				if (b && it->getVal().size() >= n * sizeof(T))
					return it->getVal().data();
				else
					return nullptr;
			});

			if (data){
				const auto *cv = cast_cv<T>(data);

				uint64_t const x = betoh<T>(cv[n]);

				return result.set(x);
			}else{
				return result.set_0();
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvget",	"CVGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CVMGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			// cvget key 32 0 1 2 3 4
			if (p.size() < 4)
				return;

			using namespace cv_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto &container  = blob.container;
			auto &bcontainer = blob.bcontainer;

			auto const varg = 3;

			if (container.capacity() < p.size() - varg)
				return;

			container.clear();
			bcontainer.clear();

			auto const t = from_string<uint8_t>(p[2]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
				}else{
					return process_<T>(key, p, *db, result, container, bcontainer);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T, typename Container, typename BContainer>
		void process_(std::string_view key, ParamContainer const &p, typename DBAdapter::List &list, Result<Protocol> &result, Container &container, BContainer &bcontainer) const{
			using namespace cv_impl_;

			auto const varg = 3;

			const auto *pair = hm4::getPairPtr(list, key);

			if (pair){
				auto const data_size = pair->getVal().size();

				const auto *cv = cast_cv<T>(pair->getVal().data());

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					if (const auto n = from_string<size_type>(*itk); data_size >= n * sizeof(T)){
						auto const x = betoh<T>(cv[n]);

						bcontainer.push_back();

						container.push_back( to_string(x, bcontainer.back()) );
					}else{
						container.emplace_back("0");
					}
				}

				return result.set_container(container);
			}else{
				// return zeroes
				container.assign(p.size() - varg, "0");

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvmget",	"CVMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CVMAX : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			// cvmax 32
			if (p.size() != 2)
				return;

			auto const t = from_string<uint8_t>(p[1]);

			using namespace cv_impl_;

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
				}else{
					return result.set(CV_MAX<T>);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvmax",	"CVMAX"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
			//	CVSET		,
				CVGET		,
				CVMGET		,
				CVMAX
			>(pack);
		}
	};



} // namespace


