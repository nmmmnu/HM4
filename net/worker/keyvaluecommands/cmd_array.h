#include "base.h"

#include "smart_memcpy.h"
#include "pair_vfactory.h"

#include <stdexcept>

namespace net::worker::commands::CV{
	namespace cv_impl_{
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
		class MyArray{
			T	*p_;
			size_t	 n_;

			constexpr static bool CHECK_SIZE = true;

		public:
			constexpr MyArray(void *ptr, size_t size) :
					p_(reinterpret_cast<T *>(ptr)	),
					n_(MyArray::size(size)		){}

			constexpr MyArray(const void *ptr, size_t size) :
					p_(reinterpret_cast<T *>(ptr)	),
					n_(MyArray::size(size)		){}

			explicit
			constexpr MyArray(std::string_view s) :
					MyArray(s.data(), s.size()){}

		public:
			constexpr static size_t size(size_t size){
				return size / sizeof(T);
			}

			constexpr static size_t bytes(size_t n){
				return n * sizeof(T);
			}

		public:
			constexpr size_t size() const{
				return n_;
			}

			constexpr size_t bytes() const{
				return bytes(n_);
			}

			constexpr const auto&operator[](size_t i) const{
				checkSize_(i);

				return p_[i];
			}

			constexpr auto&operator[](size_t i){
				checkSize_(i);

				return p_[i];
			}

		private:
			constexpr void checkSize_(size_t i) const{
				if constexpr(CHECK_SIZE){
					if (i >= n_)
						throw std::logic_error("MyArray overrun");
				}
			}
		};



		template<typename T>
		constexpr size_t size_cv(size_t const size){
			return MyArray<const T>::size(size);
		}

		template<typename T>
		constexpr size_t size_cv(std::string_view s){
			return size_cv<T>(s.size());
		}

		template<typename T>
		constexpr size_t size_cv(const hm4::Pair *p){
			return p ? size_cv<T>(p->getVal()) : 0;
		}

		template<typename T>
		constexpr size_t bytes_cv(size_t n){
			return MyArray<const T>::bytes(n);
		}

		template<typename T>
		constexpr std::string_view bytes_fix(std::string_view v){
			return {
				v.data(),
				bytes_cv<T>( size_cv<T>(v.size()) )
			};
		}

		template<typename T>
		constexpr size_type CV_MAX		= size_cv<T>(hm4::PairConf::MAX_VAL_SIZE);
	}



	template<class Protocol, class DBAdapter>
	struct CVPUSH : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// cvpush key 32 1
			if (p.size() < 4)
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

			const auto *pair = hm4::getPairPtr(list, key);

			auto const &val = pair ? bytes_fix<T>(pair->getVal()) : "";

			auto const [ok, n_size] = getNSize_<T>(p, val);

			if (!ok)
				return;

			using MyCVPUSH_Factory = CVPUSH_Factory<T, ParamContainer::iterator>;

			auto const varg = 3;

			if (pair && hm4::canInsertHint(list, pair, n_size))
				hm4::proceedInsertHintV<MyCVPUSH_Factory>(list, pair, key, n_size, std::begin(p) + varg, std::end(p), pair);
			else
				hm4::insertV<MyCVPUSH_Factory>(list, key, n_size, std::begin(p) + varg, std::end(p), pair);

			return result.set();
		}

		template<typename T>
		static auto getNSize_(ParamContainer const &p, std::string_view const val){
			using namespace cv_impl_;

			auto const varg = 3;

			size_type const len = size_cv<T>(val) - 1 + p.size() - varg;

			bool const ok = len < CV_MAX<T>;

			// max is index, but in this case, we want size
			return std::make_pair(ok, bytes_cv<T>(len + 1) );
		}

	private:
		template<typename T, typename It>
		struct CVPUSH_Factory : hm4::PairFactory::IFactory{
			using Pair = hm4::Pair;

			CVPUSH_Factory(std::string_view const key, uint64_t val_size, It begin, It end, const Pair *pair) :
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

			void createHint(Pair *) final{
				throw std::logic_error("Must not call CVPUSH_Factory::createHint");
			//	add_(pair);
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
					smart_memcpy<1,0>(data, val_size, old_pair->getVal());
				}else{
					memset(data, '\0', val_size);
				}
			}

			void add_(Pair *pair) const{
				using namespace cv_impl_;

				auto br = MyArray<T>(pair->getValC(), pair->getVal().size());

				size_type len = old_pair ? size_cv<T>(old_pair->getVal()) : 0;

				for(auto it = begin; it != end; ++it){
					T const value = from_string<T>(*it);

					br[len++] = htobe<T>(value);
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
			"cvpush",	"CVPUSH"	,
			"cvmpush",	"CVMPUSH"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CVPOP : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// cvpop key 32
			if (p.size() != 3)
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
					return process_<T>(key, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cv_impl_;

			const auto *pair = hm4::getPairPtr(list, key);

			auto const len = size_cv<T>(pair);

			if (len == 0)
				return result.set_0();

			auto const br = MyArray<const T>{ pair->getVal() };

			uint64_t const r = betoh<T>( br[len - 1] );

			using MySetSize_Factory = hm4::PairFactory::SetSize;

			auto const bytes = bytes_cv<T>(len - 1);

			if (pair && hm4::canInsertHint(list, pair, bytes))
				hm4::proceedInsertHintV<MySetSize_Factory>(list, pair, key, bytes, pair);
			else
				hm4::insertV<MySetSize_Factory>(list, key, bytes, pair);

			return result.set(r);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvpop",	"CVPOP"
		};
	};



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
			return std::make_pair(ok, bytes_cv<T>(max + 1) );
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

					auto br = MyArray<T>(data, val_size);

					br[n] = htobe<T>(value);
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

			const auto n	= from_string<size_type>(p[3]);

			auto const val	= hm4::getPairVal(list, key);
			auto const len	= size_cv<T>(val);

			if (len > n){
				auto const br = MyArray<const T>{ val };

				uint64_t const r = betoh<T>( br[n] );

				return result.set(r);
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
				auto const len = size_cv<T>(pair);
				auto const br = MyArray<const T>{ pair->getVal() };

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					if (const auto n = from_string<size_type>(*itk); len > n){
						auto const r = betoh<T>( br[n] );

						bcontainer.push_back();

						container.push_back( to_string(r, bcontainer.back()) );
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
		constexpr inline static std::string_view name	= "cv";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CVPUSH		,
				CVPOP		,
				CVSET		,
				CVGET		,
				CVMGET		,
				CVLEN		,
				CVMAX
			>(pack);
		}
	};



} // namespace


