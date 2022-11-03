#include "base.h"
#include "hyperloglog.h"
#include "logger.h"

namespace net::worker::commands::HLL{

	namespace hll_impl_{

		constexpr auto LL_HLL = LogLevel::WARNING;

		constexpr uint8_t HLL_Bits = 12;

		constexpr auto getHLL(){
			return hyperloglog::HyperLogLogRaw{HLL_Bits};
		}

		constexpr uint32_t HLL_M = getHLL().m;

		template<class DBAdapter>
		auto store(DBAdapter &db, std::string_view key, const uint8_t *hll){
			return db.set(
				key,
				std::string_view{
					reinterpret_cast<const char *>(hll),
					HLL_M
				}
			);
		}

		template<class DBAdapter>
		auto store(DBAdapter &db, const hm4::Pair *pair){
			// update only timestamps
			return db.expHint(pair, 0);
		}

		template<class DBAdapter>
		const hm4::Pair *load_pair(DBAdapter &db, std::string_view key){
			auto it = db.find(key);

			if (it && it->isValid(std::true_type{}) && it->getVal().size() == HLL_M)
				return & *it;
			else
				return nullptr;
		}

		template<class DBAdapter>
		const uint8_t *load_ptr(DBAdapter &db, std::string_view key){
			const auto *pair =  load_pair(db, key);

			if (pair)
				return reinterpret_cast<const uint8_t *>(pair->getVal().data());
			else
				return nullptr;
		}

		template<class DBAdapter>
		double hll_op_intersect(MySpan<std::string_view> const &keys, DBAdapter &db){
			StaticVector<const uint8_t *, 5> b;

			for(auto it = std::begin(keys); it != std::end(keys); ++it)
				if (const auto *x = load_ptr(db, *it); x){
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
	}



	template<class DBAdapter>
	struct PFADD : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pfadd";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"pfadd",	"PFADD"		,
			"hlladd",	"HLLADD"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() < 3)
				return Result::error();

			auto const &key = p[1];

			if (key.empty())
				return Result::error();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return Result::error();

			auto add_values = [&p](auto &hll){
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					const auto &val = *itk;

					using namespace hll_impl_;

					getHLL().add(hll, val);
				}
			};

			using namespace hll_impl_;

			const auto *pair = load_pair(db, key);

			if (pair == nullptr){
				// invalid key.

				uint8_t hll[HLL_M];

				getHLL().clear(hll);

				add_values(hll);

				store(db, key, hll);

				return Result::ok_1();
			}else if (db.canUpdateWithHint(pair)){
				auto cast = [](const char *s){
					const uint8_t *c = reinterpret_cast<const uint8_t *>(s);

					return const_cast<uint8_t *>(c);
				};

				uint8_t *hll = cast(pair->getVal().data());

				add_values(hll);

				store(db, pair);

				return Result::ok_1();
			}else{
				// proceed with normal update

				uint8_t hll[HLL_M];

				const uint8_t *b = reinterpret_cast<const uint8_t *>(pair->getVal().data());

				// copy b -> hll
				getHLL().load(hll, b);

				add_values(hll);

				store(db, key, hll);

				return Result::ok_1();
			}
		}
	};



	template<class DBAdapter>
	struct PFINTERSECT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pfintersect";
		constexpr inline static std::string_view cmd[]	= {
			"pfintersect",	"PFINTERSECT"		,
			"hllintersect",	"HLLINTERSECT"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{

			// we support just "PFINTERSECT" without arguments
			if (p.size() == 1)
				return Result::ok_0();

			// we support intersect of up to 5 sets
			if (p.size() > 6)
				return Result::error();

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			MySpan<std::string_view> const &keys{ p.data() + 1, p.size() - 1 };

			using namespace hll_impl_;

			uint64_t const n = hll_op_round(
						hll_op_intersect(keys, db)
			);

			return Result::ok( n );
		}
	};



	template<class DBAdapter>
	struct PFCOUNT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pfcount";
		constexpr inline static std::string_view cmd[]	= {
			"pfcount",	"PFCOUNT"		,
			"hllcount",	"HLLCOUNT"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{

			// we support just "PFCOUNT" without arguments
			if (p.size() == 1)
				return Result::ok_0();

		//	if (p.size() < 2)
		//		return Result::error();

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			using namespace hll_impl_;

			uint8_t hll[HLL_M];

			getHLL().clear(hll);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto *b = load_ptr(db, *itk); b)
					getHLL().merge(hll, b);

			uint64_t const n = hll_op_round(
						getHLL().estimate(hll)
			);

			return Result::ok( n );
		}

	};



	template<class DBAdapter>
	struct PFMERGE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pfmerge";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"pfmerge",	"PFMERGE"		,
			"hllmerge",	"HLLMERGE"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() < 3)
				return Result::error();

			const auto &dest_key = p[1];

			if (dest_key.empty())
				return Result::error();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			using namespace hll_impl_;

			uint8_t hll[HLL_M];

			getHLL().clear(hll);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto *b = load_ptr(db, *itk); b)
					getHLL().merge(hll, b);

			store(db, dest_key, hll);

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct PFBITS : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pfbits";
		constexpr inline static std::string_view cmd[]	= {
			"pfbits",	"PFBITS"		,
			"hllbits",	"HLLBITS"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			using namespace hll_impl_;

			return Result::ok(uint64_t{HLL_Bits});
		}
	};



	template<class DBAdapter>
	struct PFERROR : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "pferror";
		constexpr inline static std::string_view cmd[]	= {
			"pferror",	"PFERROR"		,
			"hllerror",	"HLLERROR"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			using namespace hll_impl_;

			return Result::ok(static_cast<uint64_t>(getHLL().error() * 10000));
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				PFADD		,
				PFCOUNT		,
				PFINTERSECT	,
				PFMERGE		,
				PFBITS		,
				PFERROR
			>(pack);
		}
	};


} // namespace


