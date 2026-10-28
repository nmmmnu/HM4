#include "base.h"
#include "hyperloglog.h"
#include "logger.h"
#include "pair_vfactory.h"
#include "shared_hint.h"
#include "stringtokenizer.h"

namespace net::worker::commands::HLL{
	namespace impl_{
		using Pair = hm4::Pair;

		constexpr uint8_t HLL_Bits = 12;

		constexpr auto createHLL(){
			return hyperloglog::HyperLogLogRaw{HLL_Bits};
		}

		constexpr uint32_t HLL_M = createHLL().m;

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

		constexpr uint64_t hll_op_round(double const estimate){
			return estimate < 0.1 ? 0 : static_cast<uint64_t>(round(estimate));
		}



		template<size_t N>
		static std::string_view formatDouble(double n, std::array<char, N> &buffer){
			constexpr static auto fmt_mask = FMT_COMPILE("{:+.10f}");

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, n);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		}

	} // namespace impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFADD : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PFADD() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PFADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, HLL_M);

			PFADDFactory factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			return result.set(factory.getBits());
		}

		struct PFADDFactory : hm4::PairFactory::IFactoryAction<1,1,PFADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFADDFactory>;

			using It   = ParamContainer::iterator;

			PFADDFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, impl_::HLL_M, pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				bits = action_(pair);
			}

			constexpr auto getBits() const{
				return bits;
			}



			bool action_(Pair *pair) const{
				using namespace impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				auto hll = createHLL();

				bool result = false;

				for(auto itk = begin; itk != end; ++itk)
					result |= hll.add(hll_data, *itk);

				return result;
			}

			It	begin;
			It	end;

			bool	bits = false;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfadd",	"PFADD"		,
			"hlladd",	"HLLADD"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFADDCOUNT : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PFADDCOUNT() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PFADDCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, HLL_M);

			PFADDCOUNTFactory factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			uint64_t const n = hll_op_round(
						factory.getCount()
			);

			return result.set(n);
		}

		// mostly copy of PFADDFactory but add some operations
		struct PFADDCOUNTFactory : hm4::PairFactory::IFactoryAction<1,1,PFADDCOUNTFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFADDCOUNTFactory>;

			using It   = ParamContainer::iterator;

			PFADDCOUNTFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, impl_::HLL_M, pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				this->count = action_(pair);
			}

			constexpr auto getCount() const{
				return count;
			}

			double action_(Pair *pair) const{
				using namespace impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				auto hll = createHLL();

				for(auto itk = begin; itk != end; ++itk){
					const auto &val = *itk;

					[[maybe_unused]]
					auto const bits = hll.add(hll_data, val);
				}

				return hll.estimate(hll_data);
			}

			It	begin;
			It	end;

			double	count = 0;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfaddcount",	"PFADDCOUNT"		,
			"hlladdcount",	"HLLADDCOUNT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFSET : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PFSET() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PFSET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		// PFSET key delimiter "words,words"
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const delimiter = p[2];

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const tokens = p[3];

			// not checking size

			using namespace impl_;

			const Pair *pair = nullptr;

			PFSETFactory factory{ key, pair, delimiter[0], tokens };

			insertHintVFactory(*db, pair, factory);

			uint64_t const n = hll_op_round(
						factory.getCount()
			);

			return result.set(n);
		}

		struct PFSETFactory : hm4::PairFactory::IFactoryAction<0,1,PFSETFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,1,PFSETFactory>;

			PFSETFactory(std::string_view const key, const Pair *pair, char delimiter, std::string_view tokens) :
							Base::IFactoryAction	(key, impl_::HLL_M, pair	),
							delimiter		(delimiter			),
							tokens			(tokens				){}

			void action(Pair *pair){
				this->count = action_(pair);
			}

			constexpr auto getCount() const{
				return count;
			}

			double action_(Pair *pair) const{
				using namespace impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				auto hll = createHLL();

				hll.clear(hll_data);

				StringTokenizer const tok{ tokens, delimiter };

				for(auto const &val : tok){
					if (!val.size())
						continue;

					[[maybe_unused]]
					auto const bits = hll.add(hll_data, val);
				}

				return hll.estimate(hll_data);
			}

			char			delimiter;
			std::string_view	tokens;

			double			count = 0;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfset",	"PFSET"		,
			"hllset",	"HLLSET"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFRESERVE : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PFRESERVE() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PFRESERVE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, HLL_M);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfreserve",	"PFRESERVE"		,
			"hllreserve",	"HLLRESERVE"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFINTERSECT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFINTERSECT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFINTERSECT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){

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

			using namespace impl_;

			uint64_t const n = hll_op_round(
						intersect_(keys, db)
			);

			return result.set(n);
		}

		double intersect_(MySpan<const std::string_view> const &keys, DBAdapter &db){
			using namespace impl_;

			StaticVector<const uint8_t *, 5> b;

			for(auto it = std::begin(keys); it != std::end(keys); ++it)
				if (const auto *x = load_ptr(*db, *it); x){
					b.push_back(x);

					logger<Logger::DEBUG>() << "HLL Operation" << "intersect" << *it;
				}else{
					// empty set = guaranteed zero

					return 0;
				}

			logger<Logger::DEBUG>() << "HLL Operation count" << b.size();

			auto hll_ops = createHLL().getOperations();

			auto &_ = buffer_;

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

	private:
		uint8_t buffer_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfintersect",	"PFINTERSECT"		,
			"hllintersect",	"HLLINTERSECT"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFJACCARD : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFJACCARD() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFJACCARD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		// PFJACCARD key otherkey
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &otherKey = p[2];

			if (!hm4::Pair::isKeyValid(otherKey))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *hllA = load_ptr(*db, key);

			if (!hllA)
				return result.set_0();

			const auto *hllB = load_ptr(*db, otherKey);

			if (!hllB)
				return result.set_0();

			auto hll_ops = createHLL().getOperations();

			auto &_ = buffer_;

			to_string_buffer_t buffer;

			return result.set(
				formatDouble(
					hll_ops.jaccard(_, hllA, hllB),
					buffer
				)
			);
		}

	private:
		uint8_t buffer_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfjaccard",	"PFJACCARD"		,
			"hlljaccard",	"HLLJACCARD"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFMJACCARD : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFMJACCARD() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFMJACCARD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process_(p, db, result, blob);
		}

	private:
		// PFJACCARD key otherkey...
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 2;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *hllA = load_ptr(*db, key);

			if (!hllA){
				// main HLL does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto hll_ops = createHLL().getOperations();

			auto &_ = buffer_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *hllB = load_ptr(*db, *itk);

				if (!hllB){
					container.push_back("0");

					continue;
				}

				bcontainer.push_back();

				container.push_back(
					formatDouble(
						hll_ops.jaccard(_, hllA, hllB),
						bcontainer.back()
					)
				);
			}

			return result.set_container(container);
		}

	private:
		uint8_t buffer_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfmjaccard",	"PFMJACCARD"		,
			"hllmjaccard",	"HLLMJACCARD"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFOVERLAP : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFOVERLAP() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFOVERLAP", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		// PFOVERLAP key otherkey
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){


			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto &otherKey = p[2];

			if (!hm4::Pair::isKeyValid(otherKey))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *hllA = load_ptr(*db, key);

			if (!hllA)
				return result.set_0();

			const auto *hllB = load_ptr(*db, otherKey);

			if (!hllB)
				return result.set_0();

			auto hll_ops = createHLL().getOperations();

			auto &_ = buffer_;

			to_string_buffer_t buffer;

			return result.set(
				formatDouble(
					hll_ops.overlap(_, hllA, hllB),
					buffer
				)
			);
		}

	private:
		uint8_t buffer_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfoverlap",	"PFOVERLAP"		,
			"hlloverlap",	"HLLOVERLAP"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFMOVERLAP : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFMOVERLAP() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFMOVERLAP", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process_(p, db, result, blob);
		}

	private:
		// PFOVERLAP key otherkey...
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){

			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 2;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto &container = blob.construct<OutputBlob::Container>();

			const auto *hllA = load_ptr(*db, key);

			if (!hllA){
				// main HLL does not exists

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0");

				return result.set_container(container);
			}

			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto hll_ops = createHLL().getOperations();

			auto &_ = buffer_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto *hllB = load_ptr(*db, *itk);

				if (!hllB){
					container.push_back("0");

					continue;
				}

				bcontainer.push_back();

				container.push_back(
					formatDouble(
						hll_ops.overlap(_, hllA, hllB),
						bcontainer.back()
					)
				);
			}

			return result.set_container(container);
		}

	private:
		uint8_t buffer_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfmoverlap",	"PFMOVERLAP"		,
			"hllmoverlap",	"HLLMOVERLAP"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFCOUNT : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFCOUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFCOUNT", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process_(p, db, result);
		}

	private:
		void process_(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){

			// we support just "PFCOUNT" without arguments
			if (p.size() == 1)
				return result.set_0();

		//	if (p.size() < 2)
		//		return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			createHLL().clear(hll_);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto *b = load_ptr(*db, *itk); b)
					createHLL().merge(hll_, b);

			uint64_t const n = hll_op_round(
						createHLL().estimate(hll_)
			);

			return result.set( n );
		}

	private:
		uint8_t hll_[impl_::HLL_M];

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfcount",	"PFCOUNT"		,
			"hllcount",	"HLLCOUNT"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFMERGE : BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PFMERGE() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PFMERGE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

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

			insertHintVFactory(*db, pair, factory);

			return result.set();
		}

		using HLLVector = StaticVector<const uint8_t *, OutputBlob::ParamContainerSize>;

		struct PFMergeFactory : hm4::PairFactory::IFactoryAction<1,1,PFMergeFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,PFMergeFactory>;

			using It = HLLVector::iterator;

			PFMergeFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, impl_::HLL_M, pair),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair) const{
				using namespace impl_;

				uint8_t *hll_data = hm4::getValAs<uint8_t>(pair);

				auto hll = createHLL();

				// This is fine, because flush list give guarantees now.

				for(auto it = begin; it != end; ++it)
					hll.merge(hll_data, *it);
			}

			It	begin;
			It	end;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfmerge",	"PFMERGE"		,
			"hllmerge",	"HLLMERGE"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFBITS : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFBITS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFBITS", std::begin(cmd__), std::end(cmd__)){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			return result.set(uint64_t{HLL_Bits});
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pfbits",	"PFBITS"		,
			"hllbits",	"HLLBITS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PFERROR : BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PFERROR() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PFERROR", std::begin(cmd__), std::end(cmd__)){}

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			using namespace impl_;

			to_string_buffer_t buffer;

			return result.set(
				formatDouble(
					createHLL().error(),
					buffer
				)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pferror",	"PFERROR"		,
			"hllerror",	"HLLERROR"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				PFADD		,
				PFADDCOUNT	,
				PFSET		,
				PFRESERVE	,
				PFCOUNT		,
				PFINTERSECT	,
				PFJACCARD	,
				PFMJACCARD	,
				PFOVERLAP	,
				PFMOVERLAP	,
				PFMERGE		,
				PFBITS		,
				PFERROR
			>(pack);
		}
	};


} // namespace


