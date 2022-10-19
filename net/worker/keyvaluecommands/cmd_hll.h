#include "base.h"
#include "hyperloglog.h"
#include "logger.h"

namespace net::worker::commands::HLL{

	namespace hll_impl_{

		constexpr auto LL_HLL = LogLevel::NOTICE;

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

		enum class hll_op_operation{
			merge		,
			intersect
		};

		template<hll_op_operation Op, class DBAdapter>
		double hll_op(MySpan<std::string_view> const &keys, DBAdapter &db){
			const uint8_t *b[5];

			size_t count = 0;

			for(auto it = std::begin(keys); it != std::end(keys); ++it){
				b[count] = load_ptr(db, *it);
				if (b[count]){
					++count;

					if constexpr(1){
						if constexpr(Op == hll_op_operation::merge)
							log__<LL_HLL>("HLL Operation", "merge", *it);

						if constexpr(Op == hll_op_operation::intersect)
							log__<LL_HLL>("HLL Operation", "intersect", *it);
					}
				}
			}

			log__<LL_HLL>("HLL Operation count", count);

			auto estimator = [&](){
				auto hll_ops = getHLL().getOperations();

				uint8_t _[HLL_M];

				if constexpr(Op == hll_op_operation::merge){
					switch(count){
					default:
					case 0: return hll_ops.merge(_	 					);
					case 1: return hll_ops.merge(_, b[0]					);
					case 2: return hll_ops.merge(_, b[0], b[1]				);
					case 3: return hll_ops.merge(_, b[0], b[1], b[2]			);
					case 4: return hll_ops.merge(_, b[0], b[1], b[2], b[3]			);
					case 5: return hll_ops.merge(_, b[0], b[1], b[2], b[3], b[4]		);
					}
				}

				if constexpr(Op == hll_op_operation::intersect){
					switch(count){
					default:
					case 0: return hll_ops.intersect(_	 				);
					case 1: return hll_ops.intersect(_, b[0]				);
					case 2: return hll_ops.intersect(_, b[0], b[1]				);
					case 3: return hll_ops.intersect(_, b[0], b[1], b[2]			);
					case 4: return hll_ops.intersect(_, b[0], b[1], b[2], b[3]		);
					case 5: return hll_ops.intersect(_, b[0], b[1], b[2], b[3], b[4]	);
					}
				}
			};

			return estimator();
		}

		template<hll_op_operation Op, class DBAdapter>
		uint64_t hll_op_round(MySpan<std::string_view> const &keys, DBAdapter &db){
			double const estimate = hll_op<Op>(keys, db);

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
			if (p.size() != 3)
				return Result::error();

			auto const &key = p[1];

			if (key.empty())
				return Result::error();

			auto const &val = p[2];

			if (val.empty())
				return Result::error();

			using namespace hll_impl_;

			const auto *pair = load_pair(db, key);

			if (pair == nullptr){
				// invalid key.

				uint8_t hll[HLL_M];

				getHLL().clear(hll);

				getHLL().add(hll, val);

				store(db, key, hll);

				return Result::ok(1);
			}else if (db.canUpdateWithHint(pair)){
				auto cast = [](const char *s){
					const uint8_t *c = reinterpret_cast<const uint8_t *>(s);

					return const_cast<uint8_t *>(c);
				};

				uint8_t *hll = cast(pair->getVal().data());

				getHLL().add(hll, val);

				store(db, pair);

				return Result::ok(1);

			}else{
				// proceed with normal update

				uint8_t hll[HLL_M];

				const uint8_t *b = reinterpret_cast<const uint8_t *>(pair->getVal().data());

				// copy b -> hll
				getHLL().load(hll, b);

				getHLL().add(hll, val);

				store(db, key, hll);

				return Result::ok(1);
			}
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
			if (p.size() < 2 && p.size() > 6)
				return Result::error();

			for(auto it = std::next(std::begin(p)); it != std::end(p); ++it)
				if (it->empty())
					return Result::error();

			MySpan<std::string_view> const &keys{ p.data() + 1, p.size() - 1 };

			using namespace hll_impl_;

			uint64_t const n = hll_op_round<hll_op_operation::merge>(keys, db);

			return Result::ok( n );
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
			if (p.size() < 2 && p.size() > 6)
				return Result::error();

			for(auto it = std::next(std::begin(p)); it != std::end(p); ++it)
				if (it->empty())
					return Result::error();

			MySpan<std::string_view> const &keys{ p.data() + 1, p.size() - 1 };

			using namespace hll_impl_;

			uint64_t const n = hll_op_round<hll_op_operation::intersect>(keys, db);

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
			if (p.size() < 3 && p.size() > 7)
				return Result::error();

			const auto &dest_key = p[1];

			if (dest_key.empty())
				return Result::error();

			for(auto it = std::next(std::next(std::begin(p))); it != std::end(p); ++it)
				if (it->empty())
					return Result::error();

			using namespace hll_impl_;

			uint8_t hll[HLL_M];

			getHLL().clear(hll);

			MySpan<std::string_view> const &keys{ p.data() + 2, p.size() - 2 };

			for(auto it = std::begin(keys); it != std::end(keys); ++it){
				const uint8_t *b = load_ptr(db, *it);

				if (b)
					getHLL().merge(hll, b);
			}

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

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
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

		Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
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


