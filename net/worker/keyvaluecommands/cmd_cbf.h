#include "base.h"
#include "murmur_hash_64a.h"
#include "pair_vfactory.h"
#include "shared_incr.h"

#include <algorithm>
#include <type_traits>

namespace net::worker::commands::CBF{
	namespace cbf_impl_{

		using Pair = hm4::Pair;

		constexpr auto MAX_SIZE = hm4::PairConf::MAX_VAL_SIZE;



		template<typename T>
		struct CBF{
			constexpr CBF(uint64_t width, size_t countHF) : width(width), countHF(countHF){}

			constexpr auto bytes() const{
				return width * sizeof(T);
			}

			template<bool Sign>
			void add(char *data, std::string_view item, uint64_t const n) const{
				auto *m = cast__(data);

				for(size_t i = 0; i < countHF; ++i){
					using namespace shared::incr;
					incr<Sign>( m[murmur_hash64a(item, i) % width], n);
				}
			}

			template<bool Sign>
			uint64_t add_count(char *data, std::string_view item, uint64_t const n) const{
				auto *m = cast__(data);

				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < countHF; ++i){
					using namespace shared::incr;
					auto const x = incr<Sign>( m[murmur_hash64a(item, i) % width], n);

					count = std::min<uint64_t>(count, x);
				}

				return count;
			}

			uint64_t count(const char *data, std::string_view item) const{
				const auto *m = cast__(data);

				uint64_t count = std::numeric_limits<uint64_t>::max();

				for(size_t i = 0; i < countHF; ++i){
					auto const x = m[murmur_hash64a(item, i) % width];

					count = std::min<uint64_t>(count, betoh<T>(x));
				}

				return count;
			}

		private:
			constexpr static const auto *cast__(const char *data){
				return reinterpret_cast<const T *>(data);
			}

			constexpr static auto *cast__(char *data){
				return reinterpret_cast<T *>(data);
			}

		private:
			uint64_t	width;
			size_t		countHF;
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
		struct CBFMutate{
			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
				auto const varg = 5;

				if (p.size() < 7 || (p.size() - varg) % 2 != 0)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const d = from_string<size_t	>(p[3]);
				auto const t = from_string<uint8_t	>(p[4]);

				if (w == 0 || d == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else{
						return process_(key, p, CBF<T>{ w, d }, *db, result);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			void process_(std::string_view key, ParamContainer const &p, cbf_impl_::CBF<T> cbf, typename DBAdapter::List &list, Result<Protocol> &result) const{
				using namespace cbf_impl_;

				if (cbf.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				auto const varg = 5;

				for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
					if (const auto &val = *itk; val.empty())
						return result.set_error(ResultErrorMessages::EMPTY_VAL);

				const auto *pair = hm4::getPairPtrWithSize(list, key, cbf.bytes());

				using MyCBFADD_Factory = CBFADD_Factory<T, ParamContainer::iterator>;

				MyCBFADD_Factory factory{ key, pair, cbf, std::begin(p) + varg, std::end(p) };

				insertHintVFactory(pair, list, factory);

				return result.set();
			}

		private:
			template<typename T, typename It>
			struct CBFADD_Factory : hm4::PairFactory::IFactoryAction<1,1, CBFADD_Factory<T, It> >{
				using Pair = hm4::Pair;
				using Base = hm4::PairFactory::IFactoryAction<1,1, CBFADD_Factory<T, It> >;

				using CBFT = cbf_impl_::CBF<T>;

				constexpr CBFADD_Factory(std::string_view const key, const Pair *pair, CBFT cbf, It begin, It end) :
								Base::IFactoryAction	(key, cbf.bytes(), pair),
								cbf			(cbf	),
								begin			(begin	),
								end			(end	){}

				void action(Pair *pair) const{
					char *data = pair->getValC();

					for(auto itk = begin; itk != end; itk += 2){
						auto const &val = *itk;
						auto const n    = std::max<uint64_t>( from_string<uint64_t>( *std::next(itk) ), 1);

						cbf. template add<Sign>(data, val, n);
					}
				}

			private:
				CBFT	cbf;
				It	begin;
				It	end;
			};
		};



		template<class Protocol, class DBAdapter, bool Sign>
		struct CBFMutateCount{
			void operator()(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
				if (p.size() != 7)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

				const auto &key = p[1];

				if (!hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				auto const w = from_string<uint64_t	>(p[2]);
				auto const d = from_string<size_t	>(p[3]);
				auto const t = from_string<uint8_t	>(p[4]);

				const auto &item = p[5];

				if (w == 0 || d == 0)
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

				auto f = [&](auto x) {
					using T = typename decltype(x)::type;

					if constexpr(std::is_same_v<T, std::nullptr_t>){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
					}else if (auto const itemCount = from_string<T>(p[6]); itemCount == 0){
						return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
					}else{
						return process_(key, item, itemCount, CBF<T>{ w, d }, *db, result);
					}
				};

				return type_dispatch(t, f);
			}

		private:
			template<typename T>
			void process_(std::string_view key, std::string_view const item, T const itemCount, cbf_impl_::CBF<T> cbf, typename DBAdapter::List &list, Result<Protocol> &result) const{
				using namespace cbf_impl_;

				if (cbf.bytes() > MAX_SIZE){
					// emit an error
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
				}

				const auto *pair = hm4::getPairPtrWithSize(list, key, cbf.bytes());

				using MyCBFADDCOUNT_Factory = CBFADDCOUNT_Factory<T, ParamContainer::iterator>;

				MyCBFADDCOUNT_Factory factory{ key, pair, cbf, item, itemCount };

				insertHintVFactory(pair, list, factory);

				return result.set(
					factory.getScore()
				);
			}

		private:
			template<typename T, typename It>
			struct CBFADDCOUNT_Factory : hm4::PairFactory::IFactoryAction<1,1, CBFADDCOUNT_Factory<T, It> >{
				using Pair = hm4::Pair;
				using Base = hm4::PairFactory::IFactoryAction<1,1, CBFADDCOUNT_Factory<T, It> >;

				using CBFT = cbf_impl_::CBF<T>;

				constexpr CBFADDCOUNT_Factory(std::string_view const key, const Pair *pair, CBFT cbf, std::string_view const item, T const itemCount) :
								Base::IFactoryAction	(key, cbf.bytes(), pair),
								cbf			(cbf		),
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

					return cbf. template add_count<Sign>(data, item, itemCount);
				}

			private:
				CBFT			cbf;
				std::string_view	item;
				T			itemCount;
				uint64_t		score = 0;
			};
		};

	} // namespace cbf_impl_



	template<class Protocol, class DBAdapter>
	struct CBFADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			CBFMutate<Protocol, DBAdapter, true> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfadd",	"CBFADD"	,
			"cbfincr",	"CBFINCR"	,
			"cbfincrby",	"CBFINCRBY"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFREM : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			CBFMutate<Protocol, DBAdapter, false> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfrem",	"CBFREM"	,
			"cbfdecr",	"CBFDECR"	,
			"cbfdecrby",	"CBFDECRBY"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFADDCOUNT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			CBFMutateCount<Protocol, DBAdapter, true> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfaddcount",	"CBFADDCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFREMCOUNT : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			CBFMutateCount<Protocol, DBAdapter, false> mut;

			return mut(p, db, result);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfremcount",	"CBFREMCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = from_string<uint64_t	>(p[2]);
			auto const d = from_string<size_t	>(p[3]);
			auto const t = from_string<uint8_t	>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return process_(key, CBF<T>{ w, d }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, cbf_impl_::CBF<T> cbf, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cbf_impl_;

			if (cbf.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			hm4::insertV<hm4::PairFactory::Reserve>(list, key, cbf.bytes());

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfreserve",	"CBFRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace cbf_impl_;

			if (p.size() != 6)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const w = std::max<uint64_t	>(from_string<uint64_t>(p[2]), 1);
			auto const d = std::max<uint64_t	>(from_string<uint64_t>(p[3]), 1);
			auto const t = from_string<uint8_t	>(p[4]);

			if (w == 0 || d == 0)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto &val = p[5];

			auto f = [&](auto x) {
				using T = typename decltype(x)::type;

				if constexpr(std::is_same_v<T, std::nullptr_t>){
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS); // emit an error
				}else{
					return process_<T>(key, val, CBF<T>{ w, d }, *db, result);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, std::string_view item, cbf_impl_::CBF<T> cbf, typename DBAdapter::List &list, Result<Protocol> &result) const{
			using namespace cbf_impl_;

			const auto *pair = hm4::getPairPtrWithSize(list, key, cbf.bytes());

			if (! pair)
				return result.set_0();

			auto const count = cbf.count(pair->getValC(), item);

			return result.set( count );
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfcount",	"CBFCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct CBFMCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace cbf_impl_;

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
					return process_<T>(key, p, CBF<T>{ w, d }, *db, result, blob);
				}
			};

			return type_dispatch(t, f);
		}

	private:
		template<typename T>
		void process_(std::string_view key, ParamContainer const &p, cbf_impl_::CBF<T> cbf, typename DBAdapter::List &list, Result<Protocol> &result, OutputBlob &blob) const{
			using namespace cbf_impl_;

			auto const varg = 5;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(list, key, cbf.bytes());

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

					auto const n = cbf.count(pair->getValC(), item);

					bcontainer.push_back();

					container.push_back( to_string(n, bcontainer.back()) );
				}
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"cbfmcount",	"CBFMCOUNT"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "cbf";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				CBFADD		,
				CBFREM		,
				CBFADDCOUNT	,
				CBFREMCOUNT	,
				CBFRESERVE	,
				CBFCOUNT	,
				CBFMCOUNT
			>(pack);
		}
	};



} // namespace



