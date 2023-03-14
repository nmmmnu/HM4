#include "base.h"
#include "hyperloglog.h"
#include "logger.h"
#include "pair_vfactory.h"

namespace net::worker::commands::HLL{

	namespace hll_impl_{
		constexpr auto LL_HLL = LogLevel::WARNING;

		using Pair = hm4::Pair;

		constexpr uint8_t HLL_Bits = 12;

		constexpr auto getHLL(){
			return hyperloglog::HyperLogLogRaw{HLL_Bits};
		}

		constexpr uint32_t HLL_M = getHLL().m;

		template<class List>
		const uint8_t *load_ptr(List &list, std::string_view key){
			return hm4::getPair_(list, key, [](bool b, auto it) -> const uint8_t *{
				if (b && it->getVal().size() == HLL_M)
					return reinterpret_cast<const uint8_t *>(it->getVal().data());
				else
					return nullptr;
			});
		}

		template<class DBAdapter>
		double hll_op_intersect(MySpan<std::string_view> const &keys, DBAdapter &db){
			StaticVector<const uint8_t *, 5> b;

			for(auto it = std::begin(keys); it != std::end(keys); ++it)
				if (const auto *x = load_ptr(*db, *it); x){
					b.push_back(x);

					if constexpr(1)
						log__<LL_HLL>("HLL Operation", "intersect", *it);
				}

			log__<LL_HLL>("HLL Operation count", b.size());

			auto hll_ops = getHLL().getOperations();

			uint8_t _[HLL_M];

			switch(b.size()){
			default:
			case 0: return hll_ops.intersect(_	 				);
			case 1: return hll_ops.intersect(_, b[0]				);
			case 2: return hll_ops.intersect(_, b[0], b[1]				);
			case 3: return hll_ops.intersect(_, b[0], b[1], b[2]			);
			case 4: return hll_ops.intersect(_, b[0], b[1], b[2], b[3]		);
			case 5: return hll_ops.intersect(_, b[0], b[1], b[2], b[3], b[4]	);
			}
		}

		uint64_t hll_op_round(double const estimate){
			return estimate < 0.1 ? 0 : static_cast<uint64_t>(round(estimate));
		}



		template<typename It>
		struct HLLADD_Factory : hm4::PairFactory::IFactory{
			HLLADD_Factory(std::string_view const key, It begin, It end, const Pair *pair) :
							key		(key		),
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
				if (pair->getVal().size() != val_size){
					Pair::createInRawMemory<0,0>(pair, key, val_size, 0, 0);
					create_(pair);
				}

				add_(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0>(pair, key, val_size, 0, 0);
				create_(pair);

				add_(pair);
			}

		private:
			void create_(Pair *pair) const{
				char *data = const_cast<char *>(pair->getVal().data());

				if (old_pair){
					memcpy(data, old_pair->getVal().data(), val_size);
				}else{
					memset(data, '\0', val_size);
				}
			}

			void add_(Pair *pair) const{
				char    *data     = const_cast<char *		>(pair->getVal().data());
				uint8_t *hll_data = reinterpret_cast<uint8_t *	>(data);

				for(auto itk = begin; itk != end; ++itk){
					const auto &val = *itk;

					getHLL().add(hll_data, val);
				}
			}

		private:
			constexpr static auto val_size = HLL_M;

		private:
			std::string_view	key;
			It			begin;
			It			end;
			const Pair		*old_pair;
		};



		template<typename It>
		struct HLLMERGE_Factory : hm4::PairFactory::IFactory{
			HLLMERGE_Factory(std::string_view const key, It begin, It end, const Pair *pair) :
							key		(key		),
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
				if (pair->getVal().size() != val_size)
					Pair::createInRawMemory<0,0>(pair, key, val_size, 0, 0);

				add_(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0>(pair, key, val_size, 0, 0);

				add_(pair);
			}

		private:
			void add_(Pair *pair) const{
				char    *data     = const_cast<char *		>(pair->getVal().data());
				uint8_t *hll_data = reinterpret_cast<uint8_t *	>(data);

				getHLL().clear(hll_data);

				for(auto itk = begin; itk != end; ++itk){
					const auto &val = *itk;

					getHLL().add(hll_data, val);
				}
			}

		private:
			constexpr static auto val_size = HLL_M;

		private:
			std::string_view	key;
			It			begin;
			It			end;
			const Pair		*old_pair;
		};
	}



	template<class Protocol, class DBAdapter>
	struct PFADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 3)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return;

			using namespace hll_impl_;

			const auto *pair = hm4::getPair_(*db, key, [](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == HLL_M)
					return & *it;
				else
					return nullptr;
			});

			using MyHLLADD_Factory = HLLADD_Factory<ParamContainer::iterator>;

			if (pair && hm4::canInsertHint(*db, pair, HLL_M))
				hm4::proceedInsertHintV<MyHLLADD_Factory>(*db, pair, key, std::begin(p) + varg, std::end(p), pair);
			else
				hm4::insertV<MyHLLADD_Factory>(*db, key, std::begin(p) + varg, std::end(p), pair);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfadd",	"PFADD"		,
			"hlladd",	"HLLADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 2)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			using namespace hll_impl_;

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, HLL_M);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfreserve",	"PFRESERVE"		,
			"hllreserve",	"HLLRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFINTERSECT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{

			// we support just "PFINTERSECT" without arguments
			if (p.size() == 1)
				return result.set_0();

			// we support intersect of up to 5 sets
			if (p.size() > 6)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			MySpan<std::string_view> const &keys{ p.data() + 1, p.size() - 1 };

			using namespace hll_impl_;

			uint64_t const n = hll_op_round(
						hll_op_intersect(keys, db)
			);

			return result.set( n );
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfintersect",	"PFINTERSECT"		,
			"hllintersect",	"HLLINTERSECT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{

			// we support just "PFCOUNT" without arguments
			if (p.size() == 1)
				return result.set_0();

		//	if (p.size() < 2)
		//		return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			using namespace hll_impl_;

			uint8_t *hll = hll_;

			getHLL().clear(hll);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto *b = load_ptr(*db, *itk); b)
					getHLL().merge(hll, b);

			uint64_t const n = hll_op_round(
						getHLL().estimate(hll)
			);

			return result.set( n );
		}

	private:
		uint8_t hll_[hll_impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfcount",	"PFCOUNT"		,
			"hllcount",	"HLLCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFMERGE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return;

			const auto &dest_key = p[1];

			if (dest_key.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			auto &container = blob.container;

			if (container.capacity() < p.size())
				return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &key = *itk;

				container.push_back(
					hm4::getPairVal(*db, key)
				);
			}

			using namespace hll_impl_;

			using MyHLLMERGE_Factory = HLLMERGE_Factory<ParamContainer::iterator>;

			hm4::insertV<MyHLLMERGE_Factory>(*db, key, std::begin(p) + varg, std::end(p), pair);




			uint8_t *hll = hll_;

			getHLL().clear(hll);

			// this is not good candidate for a factory,
			// because it will read and write at the same time.

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto *b = load_ptr(*db, *itk); b)
					getHLL().merge(hll, b);

			store(*db, dest_key, hll);

			return result.set();
		}

	private:
		uint8_t hll_[hll_impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfmerge",	"PFMERGE"		,
			"hllmerge",	"HLLMERGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFBITS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace hll_impl_;

			return result.set(uint64_t{HLL_Bits});
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfbits",	"PFBITS"		,
			"hllbits",	"HLLBITS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFERROR : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace hll_impl_;

			return result.set(static_cast<uint64_t>(getHLL().error() * 10000));
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pferror",	"PFERROR"		,
			"hllerror",	"HLLERROR"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				PFADD		,
				PFRESERVE	,
				PFCOUNT		,
				PFINTERSECT	,
				PFMERGE		,
				PFBITS		,
				PFERROR
			>(pack);
		}
	};


} // namespace


