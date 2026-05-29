#include "base.h"

#include "accdata.h"
#include "to_fp.h"

#include "pair_vfactory.h"

namespace net::worker::commands::SummaryStats{

	constexpr bool UseHashSearcher = true;

	namespace summary_stats_impl_{

		struct SSADDFactory : hm4::PairFactory::IFactoryAction<1,1,SSADDFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,SSADDFactory>;

			using It = ParamContainer::const_iterator;

			SSADDFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, sizeof(AccData), pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				auto *acc = hm4::getValAs<AccData>(pair);

				for(auto itk = begin; itk != end; ++itk){
					double const d = to_double_def(*itk);
					acc->accumulate(d);
				}
			}

		private:
			It	begin;
			It	end;
		};



		template<typename List>
		auto const &mergeLoadContainer(ParamContainer::const_iterator begin, ParamContainer::const_iterator end,
						List const &list, OutputBlob::PairContainer &pcontainer){
			for(auto it = begin; it != end; ++it){
				auto const &key = *it;
				const auto *pair = hm4::getPairPtrWithSize(list, key, sizeof(AccData));

				if (pair)
					pcontainer.push_back(pair);
			}

			auto sortF = [](const hm4::Pair *a, const hm4::Pair *b){
				return a->cmpTime(*b) > 0;
			};

			// max 0xFF - 1 items
			std::sort(std::begin(pcontainer), std::end(pcontainer), sortF);

			return pcontainer;
		}



		void merge(AccData &acc, OutputBlob::PairContainer::const_iterator begin, OutputBlob::PairContainer::const_iterator end){
			for(auto it = begin; it != end; ++it){
				const auto *accSub = hm4::getValAs<AccData>(*it);
				acc.merge(*accSub);
			}
		}

		AccData merge(OutputBlob::PairContainer::const_iterator begin, OutputBlob::PairContainer::const_iterator end){
			AccData acc;

			merge(acc, begin, end);

			return acc;
		}

	} // namespace summary_stats_impl_



	template<class Protocol, class DBAdapter>
	struct SSRESERVE : BaseCommandRW<Protocol,DBAdapter>{
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

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, sizeof(AccData));

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssreserve",	"SSRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SSADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ssadd key value value value...
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

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			using namespace summary_stats_impl_;

			SSADDFactory factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, *db, factory);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssadd",		"SSADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SSMERGE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ssmerge key key_to_merge key_to_merge key_to_merge...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			using namespace summary_stats_impl_;

			auto const &pcontainer = mergeLoadContainer(
				std::begin(p) + varg	,
				std::end(p)		,
				*db			,
				blob.construct<OutputBlob::PairContainer>()
			);

			SSMERGEFactory factory{ key, pair, std::begin(pcontainer), std::end(pcontainer) };

			insertHintVFactory(pair, *db, factory);

			return result.set_1();
		}

	private:
		struct SSMERGEFactory : hm4::PairFactory::IFactoryAction<0 /* DO NOT COPY VALUE */,1,SSMERGEFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<0,1,SSMERGEFactory>;

			using It = OutputBlob::PairContainer::const_iterator;

			SSMERGEFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, sizeof(AccData), pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				auto *acc = hm4::getValAs<AccData>(pair);

				acc->clear();

				using namespace summary_stats_impl_;

				merge(*acc, begin, end);
			}

		private:
			It	begin;
			It	end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssmerge",		"SSMERGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SSGETALL : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ssgetall key key key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace summary_stats_impl_;

			auto const &pcontainer = mergeLoadContainer(
				std::begin(p) + varg	,
				std::end(p)		,
				*db			,
				blob.construct<OutputBlob::PairContainer>()
			);

			AccData acc = merge(std::cbegin(pcontainer), std::cend(pcontainer));

			blob.resetAllocator();

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto _ = [&container, &bcontainer, &acc](std::string_view key, double const d){

				constexpr static std::string_view fmt_mask = "{:+.10f}";

				bcontainer.push_back();

				auto &buffer = bcontainer.back();

				auto const r = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

				if (r.out == std::end(buffer)){
					container.push_back(key);
					container.push_back("error");
				}else{
					container.push_back(key);
					container.emplace_back( buffer.data(), r.size );
				}
			};

			auto &a = acc;

			_("count"	, a.count	());
			_("open"	, a.first	());
			_("close"	, a.last	());
			_("first"	, a.first	());
			_("last"	, a.last	());
			_("min"		, a.min		());
			_("max"		, a.max		());
			_("sum"		, a.sum		());
			_("sumsq"	, a.sum2	());
			_("sum2"	, a.sum2	());
			_("range"	, a.range	());
			_("change"	, a.change	());
			_("avg"		, a.avg		());
			_("harm"	, a.harm	());
			_("geom"	, a.geom	());
			_("vari"	, a.vari	());
			_("sdev"	, a.sdev	());
			_("rms"		, a.rms		());
			_("cvar"	, a.cvar	());

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssgetall",		"SSGETALL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SSGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ssgetall key subkey
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &sub = p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			const auto *acc  = hm4::getValAs<AccData>(pair);

			auto const d = (*acc)[sub];

			constexpr static std::string_view fmt_mask = "{:+.10f}";

			OutputBlob::buffer_t buffer;

			auto const r = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (r.out == std::end(buffer)){
				constexpr std::string_view s = "0.0";
				return result.set(s);
			}else{
				std::string_view s{ buffer.data(), r.size };
				return result.set(s);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssget",		"SSGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SSMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// ssmget key subkey subkey subkey...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			// for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
			// 	const auto &subN = *itk;
			//
			// 	// no need to check
			// }

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			const auto *acc  = hm4::getValAs<AccData>(pair);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &sub = *itk;

				auto const d = (*acc)[sub];

				constexpr static std::string_view fmt_mask = "{:+.10f}";

				bcontainer.push_back();

				auto &buffer = bcontainer.back();

				auto const r = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

				if (r.out == std::end(buffer))
					container.push_back("error");
				else
					container.emplace_back(buffer.data(), r.size);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"ssmget",		"SSMGET"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "summary stats";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SSRESERVE	,
				SSMERGE		,
				SSGETALL	,
				SSGET		,
				SSMGET		,
			//	SSADDGETALL	,
			//	SSADDGET	,
			//	SSADDMGET
				SSADD
			>(pack);
		}
	};

} // namespace


