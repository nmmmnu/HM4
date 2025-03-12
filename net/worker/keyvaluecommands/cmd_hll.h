#include "base.h"
#include "hyperloglog.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"

namespace net::worker::commands::HLL{
	namespace hll_impl_{
		using Pair = hm4::Pair;

		constexpr uint8_t HLL_Bits = 12;

		constexpr auto getHLL(){
			return hyperloglog::HyperLogLogRaw{HLL_Bits};
		}

		constexpr uint32_t HLL_M = getHLL().m;

		template<class List>
		auto store(List &list, std::string_view key, const uint8_t *hll){
			return hm4::insert( list,
				key,
				std::string_view{
					reinterpret_cast<const char *>(hll),
					HLL_M
				}
			);
		}

		template<class List>
		const uint8_t *load_ptr(List &list, std::string_view key){
			if (const auto *pair = hm4::getPairPtrWithSize(list, key, HLL_M); pair)
				return hm4::getValAs<uint8_t>(pair);

			return nullptr;
		}

		template<class DBAdapter>
		double hll_op_intersect(MySpan<const std::string_view> const &keys, DBAdapter &db){
			StaticVector<const uint8_t *, 5> b;

			for(auto it = std::begin(keys); it != std::end(keys); ++it)
				if (const auto *x = load_ptr(*db, *it); x){
					b.push_back(x);

					logger<Logger::DEBUG>() << "HLL Operation" << "intersect" << *it;
				}

			logger<Logger::DEBUG>() << "HLL Operation count" << b.size();

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

		constexpr uint64_t hll_op_round(double const estimate){
			return estimate < 0.1 ? 0 : static_cast<uint64_t>(round(estimate));
		}

	} // namespace hll_impl_



	template<class Protocol, class DBAdapter>
	struct PFADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace hll_impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, HLL_M);

			PFADDFactoryBits factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, *db, factory);

			return result.set(factory.getBits());
		}

	private:
		struct PFADDFactoryBits : hm4::PairFactory::IFactoryAction<1,1,PFADDFactoryBits>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFADDFactoryBits>;

			using It = ParamContainer::iterator;

			PFADDFactoryBits(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, hll_impl_::HLL_M, pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				bits = action_(pair);;
			}

			constexpr auto getBits() const{
				return bits;
			}

		private:
			bool action_(Pair *pair) const{
				using namespace hll_impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				bool result = false;

				for(auto itk = begin; itk != end; ++itk){
					const auto &val = *itk;

					result |= getHLL().add(hll_data, val);
				}

				return result;
			}

		private:
			It	begin;
			It	end;

			bool	bits = false;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfadd",	"PFADD"		,
			"hlladd",	"HLLADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
	struct PFINTERSECT : BaseCommandRO<Protocol,DBAdapter>{
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
				return result.set_error(ResultErrorMessages::NEED_LESS_PARAMS_5);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			MySpan<const std::string_view> const &keys{ p.data() + 1, p.size() - 1 };

			using namespace hll_impl_;

			uint64_t const n = hll_op_round(
						hll_op_intersect(keys, db)
			);

			return result.set(n);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfintersect",	"PFINTERSECT"		,
			"hllintersect",	"HLLINTERSECT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFCOUNT : BaseCommandRO<Protocol,DBAdapter>{
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
		//		return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

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
	struct PFADDCOUNT : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace hll_impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, HLL_M);

			PFADDFactoryCount factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, *db, factory);

			uint64_t const n = hll_op_round(
						factory.getCount()
			);

			return result.set(n);
		}

	private:
		// mostly copy of PFADDFactory but add some operations
		struct PFADDFactoryCount : hm4::PairFactory::IFactoryAction<1,1,PFADDFactoryCount>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFADDFactoryCount>;

			using It = ParamContainer::iterator;

			PFADDFactoryCount(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, hll_impl_::HLL_M, pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				this->count = action_(pair);
			}

			constexpr auto getCount() const{
				return count;
			}

		private:
			double action_(Pair *pair) const{
				using namespace hll_impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				for(auto itk = begin; itk != end; ++itk){
					const auto &val = *itk;

					[[maybe_unused]]
					auto bits = getHLL().add(hll_data, val);
				}

				return getHLL().estimate(hll_data);
			}

		private:
			It	begin;
			It	end;

			double	count = 0;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfaddcount",	"PFADDCOUNT"		,
			"hlladdcount",	"HLLADDCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFMERGE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace hll_impl_;

			HLLVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &src_key = *itk;

				// prevent merge with itself.
				if (src_key == key)
					continue;

				if (const auto *b = load_ptr(*db, src_key); b)
					container.push_back(b);
			}

			if (container.empty())
				return result.set();

			const auto *pair = hm4::getPairPtrWithSize(*db, key, HLL_M);

			PFMergeFactory factory{ key, pair, std::begin(container), std::end(container) };

			insertHintVFactory(pair, *db, factory);

			return result.set();
		}

	private:
		using HLLVector = StaticVector<const uint8_t *, OutputBlob::ParamContainerSize>;

	private:
		struct PFMergeFactory : hm4::PairFactory::IFactoryAction<1,1,PFMergeFactory >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFMergeFactory >;

			PFMergeFactory(std::string_view const key, const Pair *pair, HLLVector::iterator begin, HLLVector::iterator end) :
							Base::IFactoryAction	(key, hll_impl_::HLL_M, pair),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair) const{
				using namespace hll_impl_;

				auto *hll = hm4::getValAs<uint8_t>(pair);

				// getHLL().clear(hll);

				// This is fine, because flush list give guarantees now.

				for(auto it = begin; it != end; ++it)
					getHLL().merge(hll, *it);
			}

		private:
			HLLVector::iterator	begin;
			HLLVector::iterator	end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"pfmerge",	"PFMERGE"		,
			"hllmerge",	"HLLMERGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PFBITS : BaseCommandRO<Protocol,DBAdapter>{
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
	struct PFERROR : BaseCommandRO<Protocol,DBAdapter>{
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
				PFADDCOUNT	,
				PFINTERSECT	,
				PFMERGE		,
				PFBITS		,
				PFERROR
			>(pack);
		}
	};


} // namespace


