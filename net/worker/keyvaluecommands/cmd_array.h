#include "base.h"

#include "smart_memcpy.h"

namespace net::worker::commands::CV{
	namespace cv_impl_{
		using Pair = hm4::Pair;

		using size_type				= uint64_t;

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

		template<typename T>
		constexpr size_t size_cv(size_t size){
			return size / sizeof(T);
		}

		template<typename T>
		constexpr size_t size_cv(std::string_view s){
			return s.size() / sizeof(T);
		}

		template<typename T>
		constexpr size_type CV_MAX		= size_cv<T>(hm4::PairConf::MAX_VAL_SIZE);
	}



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
			if (p.size() < 5 || p.size() % 2 == 0)
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

			auto const [ok, n_size] = getNSize_<T>(p);

			if (!ok)
				return;

			const auto *pair = hm4::getPairPtr(list, key);

			using MyCVSET_Factory = CVSET_Factory<T, ParamContainer::iterator>;

			auto const varg = 3;

			if (pair && hm4::canInsertHint(list, pair, n_size))
				hm4::proceedInsertHintV<MyCVSET_Factory>(list, pair, key, n_size, std::begin(p) + varg, std::end(p), pair);
			else
				hm4::insertV<MyCVSET_Factory>(list, key, n_size, std::begin(p) + varg, std::end(p), pair);

			return result.set();
		}

		template<typename T>
		static auto getNSize_(ParamContainer const &p){
			using namespace cv_impl_;

			bool ok = false;
			size_type max = 0;

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n < CV_MAX<T>){
					ok  = true;
					max = std::max(max, n);
				}
			}

			// max is index, but in this case, we want size
			return std::make_pair(ok, (max + 1) * sizeof(T) );
		}

	private:
		template<typename T, typename It>
		struct CVSET_Factory : hm4::PairFactory::IFactory{
			using Pair = hm4::Pair;

			CVSET_Factory(std::string_view const key, uint64_t val_size, It begin, It end, const Pair *pair) :
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
					smart_memcpy(data, val_size, old_pair->getVal());
				}else{
					memset(data, '\0', val_size);
				}
			}

			void add_(Pair *pair) const{
				using namespace cv_impl_;

				char *data = pair->getValC();

				for(auto it = begin; it != end; it += 2){
					auto const n = from_string<size_type>(*it);

					if (size_cv<T>(val_size) < n)
						continue;

					T const value = from_string<T>(*std::next(it));

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

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvset",	"CVSET"	,
			"cvmset",	"CVMSET"
		};
	};



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
				if (b && size_cv<T>(it->getVal()) >= n)
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
					if (const auto n = from_string<size_type>(*itk); size_cv<T>(data_size) >= n){
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
	struct CVLEN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// cvlen key 32
			if (p.size() != 3)
				return;

			using namespace cv_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const t = from_string<uint8_t>(p[2]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
				}else{
					return process_<T>(key, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cv_impl_;

			const auto val = hm4::getPairVal(list, key);

			return result.set(size_cv<T>(val));
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvlen",	"CVLEN"
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
					return process_<T>(result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(Result<Protocol> &result) const{
			using namespace cv_impl_;

			return result.set(CV_MAX<T>);
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
				CVSET		,
				CVGET		,
				CVMGET		,
				CVLEN		,
				CVMAX
			>(pack);
		}
	};



} // namespace


