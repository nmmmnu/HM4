#include "base.h"

#include "smart_memcpy.h"
#include "pair_vfactory.h"

#include <stdexcept>

namespace net::worker::commands::CV{
	namespace cv_impl_{
		constexpr bool RangeCheck	= true;

		using size_type			= uint64_t;

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



		template<typename T>
		constexpr size_t cv_size(size_t const size){
			return size / sizeof(T);
		}

		template<typename T>
		constexpr size_t cv_size(std::string_view s){
			return cv_size<T>(s.size());
		}

		template<typename T>
		constexpr size_t cv_size(const hm4::Pair *p){
			return p ? cv_size<T>(p->getVal()) : 0;
		}

		template<typename T>
		constexpr size_t cv_bytes(size_t n){
			return n * sizeof(T);;
		}

		template<typename T>
		constexpr std::string_view cv_bytes_fix(std::string_view v){
			return {
				v.data(),
				cv_bytes<T>( cv_size<T>(v.size()) )
			};
		}

		template<typename T>
		constexpr size_type CV_MAX		= cv_size<T>(hm4::PairConf::MAX_VAL_SIZE);
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

			auto const &val = pair ? cv_bytes_fix<T>(pair->getVal()) : "";

			auto const [ok, n_len] = getN__<T>(p, val);

			if (!ok)
				return;

			using MyCVPUSH_Factory = CVPUSH_Factory<T, ParamContainer::iterator>;

			auto const varg = 3;

			auto const new_val_size = cv_bytes<T>(n_len);

			// There is no way hint to be used here.
			hm4::insertV<MyCVPUSH_Factory>(list, key, new_val_size, pair, std::begin(p) + varg, std::end(p));

			return result.set();
		}

		template<typename T>
		static auto getN__(ParamContainer const &p, std::string_view val){
			using namespace cv_impl_;

			auto const varg = 3;

			size_type const len = cv_size<T>(val) - 1 + p.size() - varg;

			bool const ok = len < CV_MAX<T>;

			// max is index, but in this case, we want size
			return std::make_pair(ok, len + 1);
		}

	private:
		template<typename T, typename It>
		struct CVPUSH_Factory : hm4::PairFactory::IFactoryAction<1, 0, CVPUSH_Factory<T, It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1, 0, CVPUSH_Factory<T, It> >;

			constexpr CVPUSH_Factory(std::string_view const key, uint64_t val_size, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, val_size, pair),
							begin				(begin		),
							end				(end		){}

			void action(Pair *pair) const{
				using namespace cv_impl_;

				auto sp = blobAsMySpan<T, RangeCheck>(pair->getValC(), pair->getVal().size());

				auto &old_pair = this->old_pair;

				size_type len = old_pair ? cv_size<T>(old_pair->getVal()) : 0;

				for(auto it = begin; it != end; ++it){
					T const value = from_string<T>(*it);

					sp[len++] = htobe<T>(value);
				}
			}

		private:
			It			begin;
			It			end;
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

			auto const len = cv_size<T>(pair);

			if (len == 0)
				return result.set_0();

			auto const sp = blobAsMySpan<const T>(pair->getVal());

			uint64_t const r = betoh<T>(sp.back());

			using MySetSize_Factory = hm4::PairFactory::SetSize;

			auto const bytes = cv_bytes<T>(len - 1);

			if (pair && hm4::canInsertHintValSize(list, pair, bytes))
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

			auto const [ok, n_len] = getN__(p, CV_MAX<T>);

			if (!ok)
				return;

			const auto *pair = hm4::getPairPtr(list, key);

			using MyCVSET_Factory = CVSET_Factory<T, ParamContainer::iterator>;

			auto const varg = 3;

			auto const new_val_size = cv_bytes<T>(n_len);

			if (pair && hm4::canInsertHintValSize(list, pair, new_val_size)){
				auto const val_size = pair->getVal().size();

				hm4::proceedInsertHintV<MyCVSET_Factory>(list, pair, key, val_size, pair, std::begin(p) + varg, std::end(p));
			}else{
				auto const val_size = std::max(
							new_val_size,
							pair ? pair->getVal().size() : 0
				);

				hm4::insertV<MyCVSET_Factory>(list, key, val_size, pair, std::begin(p) + varg, std::end(p));
			}

			return result.set();
		}

		static auto getN__(ParamContainer const &p, cv_impl_::size_type const max_val){
			using namespace cv_impl_;

			bool ok = false;
			size_type max = 0;

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n < max_val){
					ok  = true;
					max = std::max(max, n);
				}
			}

			// max is index, but in this case, we want size
			return std::make_pair(ok, max + 1);
		}

	private:
		template<typename T, typename It>
		struct CVSET_Factory : hm4::PairFactory::IFactoryAction<1, 0, CVSET_Factory<T, It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1, 0, CVSET_Factory<T, It> >;

			constexpr CVSET_Factory(std::string_view const key, uint64_t val_size, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, val_size, pair),
							begin				(begin		),
							end				(end		){}

			void action(Pair *pair) const{
				using namespace cv_impl_;

				char *data = pair->getValC();

				for(auto it = begin; it != end; it += 2){
					auto const n = from_string<size_type>(*it);

					auto const val_size = this->val_size;

					if (cv_size<T>(val_size) < n)
						continue;

					T const value = from_string<T>(*std::next(it));

					auto sp = blobAsMySpan<T, RangeCheck>(data, val_size);

					sp[n] = htobe<T>(value);
				}
			}

		private:
			It			begin;
			It			end;
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
			auto const len	= cv_size<T>(val);

			if (len > n){
				auto const sp = blobAsMySpan<const T>(val);

				uint64_t const r = betoh<T>(sp[n]);

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
				auto const len = cv_size<T>(pair);
				auto const sp = blobAsMySpan<const T>(pair->getVal());

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					if (const auto n = from_string<size_type>(*itk); len > n){
						auto const r = betoh<T>(sp[n]);

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
	struct CVGETRANGE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			// cvgetrange key 32 0 100
			if (p.size() != 5)
				return;

			using namespace cv_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto &container  = blob.container;
			auto &bcontainer = blob.bcontainer;

			const auto n1	= from_string<size_type>(p[3]);
			const auto n2	= from_string<size_type>(p[4]);

			if (n1 > n2)
				return;

			if (container.capacity() < n2 - n1)
				return;

			container.clear();
			bcontainer.clear();

			auto const t = from_string<uint8_t>(p[2]);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return; // emit an error
				}else{
					return process_<T>(key, *db, result, n1, n2, container, bcontainer);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T, typename Container, typename BContainer>
		void process_(std::string_view key, typename DBAdapter::List &list, Result<Protocol> &result, cv_impl_::size_type n1, cv_impl_::size_type n2, Container &container, BContainer &bcontainer) const{
			using namespace cv_impl_;

			const auto *pair = hm4::getPairPtr(list, key);

			if (pair){
				auto const len = cv_size<T>(pair);
				auto const sp = blobAsMySpan<const T>(pair->getVal());

				for(auto i = n1; i <= n2; ++i){
					if (i < len){
						auto const r = betoh<T>(sp[i]);

						bcontainer.push_back();

						container.push_back( to_string(r, bcontainer.back()) );
					}else{
						container.emplace_back("0");
					}
				}

				return result.set_container(container);
			}else{
				// return zeroes
				container.assign(n2 - n1, "0");

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cvmetrange",	"CVGETRANGE"
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

			return result.set(cv_size<T>(val));
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
				CVGETRANGE	,
				CVLEN		,
				CVMAX
			>(pack);
		}
	};



} // namespace


