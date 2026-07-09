#include "base.h"

#include "accdata.h"
#include "to_fp.h"

#include "pair_vfactory.h"

namespace net::worker::commands::SummaryStats{

	namespace impl_{
		constexpr std::string_view ZERO = "+0.0000000000";

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

		[[nodiscard]]
		AccData merge(OutputBlob::PairContainer::const_iterator begin, OutputBlob::PairContainer::const_iterator end){
			AccData acc;

			merge(acc, begin, end);

			return acc;
		}

		[[nodiscard]]
		std::string_view convertData(double const d, OutputBlob::buffer_t &buffer){
			constexpr static std::string_view fmt_mask = "{:+.10f}";

			auto const r = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (r.out == std::end(buffer))
				return ZERO;
			else
				return { buffer.data(), r.size };
		}

		[[nodiscard]]
		std::string_view getData(AccData const &acc,
						std::string_view sub,
							OutputBlob::buffer_t &buffer){
			return convertData(acc[sub], buffer);
		}

		auto const &getData(AccData const &acc,
						ParamContainer::const_iterator begin, ParamContainer::const_iterator end,
							OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer){

			for(auto it = begin; it != end; ++it){
				const auto &sub = *it;

				bcontainer.push_back();

				auto &buffer = bcontainer.back();

				container.emplace_back(
					getData(acc, sub, buffer)
				);
			}

			return container;
		}

		[[nodiscard]]
		auto const &getDataAllZeroes(OutputBlob::Container &container){

			auto f = [&container](std::string_view sub){
				container.push_back(sub);
				container.push_back(ZERO);
			};

			AccData::for_each_key(f);

			return container;
		}

		auto const &getDataAll(AccData const &acc,
							OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer){
			if (!acc.ok())
				return getDataAllZeroes(container);

			auto f = [&container, &bcontainer](std::string_view sub, double const d){

				bcontainer.push_back();

				auto &buffer = bcontainer.back();

				container.push_back(sub);
				container.push_back(
					convertData(d, buffer)
				);
			};

			acc.for_each(f);

			return container;
		}

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct SSRESERVE : BaseCommandRW<Protocol,DBAdapter>{

		SSRESERVE() : BaseCommandRW<Protocol,DBAdapter>("SSRESERVE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, sizeof(AccData));

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssreserve",	"SSRESERVE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSADD : BaseCommandRW<Protocol,DBAdapter>{

		SSADD() : BaseCommandRW<Protocol,DBAdapter>("SSADD", std::begin(cmd__), std::end(cmd__)){}

		// ssadd key value value value...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
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

			SSADDFactory factory{ key, pair, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

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


			It	begin;
			It	end;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssadd",		"SSADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSMERGE : BaseCommandRW<Protocol,DBAdapter>{

		SSMERGE() : BaseCommandRW<Protocol,DBAdapter>("SSMERGE", std::begin(cmd__), std::end(cmd__)){}

		// ssmerge key key_to_merge key_to_merge key_to_merge...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
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

			using namespace impl_;

			auto const &pcontainer = mergeLoadContainer(
				std::begin(p) + varg	,
				std::end(p)		,
				*db			,
				blob.construct<OutputBlob::PairContainer>()
			);

			SSMERGEFactory factory{ key, pair, std::begin(pcontainer), std::end(pcontainer) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}


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

				using namespace impl_;

				merge(*acc, begin, end);
			}


			It	begin;
			It	end;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssmerge",		"SSMERGE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSGETALL : BaseCommandRO<Protocol,DBAdapter>{

		SSGETALL() : BaseCommandRO<Protocol,DBAdapter>("SSGETALL", std::begin(cmd__), std::end(cmd__)){}

		// ssgetall key key key
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; !hm4::Pair::isKeyValid(key))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (p.size() - varg == 1)
				return process_1_(p[varg], db, result, blob);
			else
				return process_N_(std::begin(p) + varg, std::end(p), db, result, blob);
		}

		static void process_1_(std::string_view key,
						DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			if (!pair){
				auto &container  = blob.construct<OutputBlob::Container>();

				return result.set_container(
					getDataAllZeroes(container)
				);
			}else{
				const auto *acc = hm4::getValAs<AccData>(pair);

				auto &container  = blob.construct<OutputBlob::Container>();
				auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

				return result.set_container(
					getDataAll(*acc, container, bcontainer)
				);
			}
		}

		static void process_N_(ParamContainer::const_iterator begin, ParamContainer::const_iterator end,
						DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

			auto &pcontainer = blob.construct<OutputBlob::PairContainer>();

			using namespace impl_;

			mergeLoadContainer(begin, end, *db, pcontainer);

			AccData acc = merge(std::cbegin(pcontainer), std::cend(pcontainer));

			blob.resetAllocator();

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			return result.set_container(
				getDataAll(acc, container, bcontainer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssgetall",		"SSGETALL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSGET : BaseCommandRO<Protocol,DBAdapter>{

		SSGET() : BaseCommandRO<Protocol,DBAdapter>("SSGET", std::begin(cmd__), std::end(cmd__)){}

		// ssgetall key subkey
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &sub = p[2];

			if (sub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			if (!pair)
				return result.set(ZERO);

			const auto *acc  = hm4::getValAs<AccData>(pair);

			OutputBlob::buffer_t buffer;

			return result.set(
				getData(*acc, sub, buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssget",		"SSGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSMGET : BaseCommandRO<Protocol,DBAdapter>{

		SSMGET() : BaseCommandRO<Protocol,DBAdapter>("SSMGET", std::begin(cmd__), std::end(cmd__)){}

		// ssmget key subkey subkey subkey...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &sub = *itk; sub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			if (!pair){
				auto &container  = blob.construct<OutputBlob::Container>();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back(ZERO);

				return result.set_container(container);
			}

			const auto *acc  = hm4::getValAs<AccData>(pair);

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			return result.set_container(
				getData(*acc, std::begin(p) + varg, std::end(p), container, bcontainer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssmget",		"SSMGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSADDGETALL : BaseCommandRW<Protocol,DBAdapter>{

		SSADDGETALL() : BaseCommandRW<Protocol,DBAdapter>("SSADDGETALL", std::begin(cmd__), std::end(cmd__)){}

		// ssaddgetall key value value value...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
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

			SSADDGETALLFactory &factory = blob.construct<SSADDGETALLFactory>(key, pair, std::begin(p) + varg, std::end(p));

			insertHintVFactory(*db, pair, factory);

			return result.set_container(
				factory.getResult()
			);
		}

		struct SSADDGETALLFactory : hm4::PairFactory::IFactoryAction<1,1,SSADDGETALLFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,SSADDGETALLFactory>;

			using It = ParamContainer::const_iterator;

			SSADDGETALLFactory(std::string_view const key, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, sizeof(AccData), pair	),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				auto *acc = hm4::getValAs<AccData>(pair);

				for(auto itk = begin; itk != end; ++itk){
					double const d = to_double_def(*itk);
					acc->accumulate(d);
				}

				using namespace impl_;

				// fill the result
				getDataAll(*acc, container, bcontainer);
			}

			[[nodiscard]]
			auto const &getResult() const{
				return container;
			}


			It				begin;
			It				end;
			OutputBlob::Container		container;
			OutputBlob::BufferContainer	bcontainer;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssaddgetall",		"SSADDGETALL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSADDGET : BaseCommandRW<Protocol,DBAdapter>{

		SSADDGET() : BaseCommandRW<Protocol,DBAdapter>("SSADDGET", std::begin(cmd__), std::end(cmd__)){}

		// ssaddget key value sub
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &val = p[2];

			if (val.empty())
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const &sub = p[3];

			if (sub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			SSADDGETFactory factory{ key, pair, val, sub };

			insertHintVFactory(*db, pair, factory);

			return result.set(
				factory.getResult()
			);
		}

		struct SSADDGETFactory : hm4::PairFactory::IFactoryAction<1,1,SSADDGETFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,SSADDGETFactory>;

			SSADDGETFactory(std::string_view const key, const Pair *pair, std::string_view val, std::string_view sub) :
							Base::IFactoryAction	(key, sizeof(AccData), pair	),
							val			(val				),
							sub			(sub				){}

			void action(Pair *pair){
				auto *acc = hm4::getValAs<AccData>(pair);

				double const d = to_double_def(val);
				acc->accumulate(d);

				using namespace impl_;

				// fill the result
				result = getData(*acc, sub, buffer);
			}

			[[nodiscard]]
			auto const &getResult() const{
				return result;
			}

			std::string_view	val;
			std::string_view	sub;

			std::string_view	result;
			OutputBlob::buffer_t	buffer;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssaddget",		"SSADDGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct SSADDMGET : BaseCommandRW<Protocol,DBAdapter>{

		SSADDMGET() : BaseCommandRW<Protocol,DBAdapter>("SSADDMGET", std::begin(cmd__), std::end(cmd__)){}

		// ssaddget key value sub sub sub...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const &val = p[2];

			if (val.empty())
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &sub = *itk; sub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, sizeof(AccData));

			SSADDMGETFactory &factory = blob.construct<SSADDMGETFactory>(key, pair, val, std::begin(p) + varg, std::end(p));

			insertHintVFactory(*db, pair, factory);

			return result.set_container(
				factory.getResult()
			);
		}

		struct SSADDMGETFactory : hm4::PairFactory::IFactoryAction<1,1,SSADDMGETFactory>{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1,1,SSADDMGETFactory>;

			using It = ParamContainer::const_iterator;

			SSADDMGETFactory(std::string_view const key, const Pair *pair, std::string_view val, It begin, It end) :
							Base::IFactoryAction	(key, sizeof(AccData), pair	),
							val			(val				),
							begin			(begin				),
							end			(end				){}

			void action(Pair *pair){
				auto *acc = hm4::getValAs<AccData>(pair);

				double const d = to_double_def(val);
				acc->accumulate(d);

				using namespace impl_;

				// fill the result
				getData(*acc, begin, end, container, bcontainer);
			}

			[[nodiscard]]
			auto const &getResult() const{
				return container;
			}


			std::string_view		val;

			It				begin;
			It				end;

			OutputBlob::Container		container;
			OutputBlob::BufferContainer	bcontainer;
		};

	private:
		constexpr inline static std::string_view cmd__[] = {
			"ssaddmget",		"SSADDMGET"
		};

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "summary stats";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SSRESERVE	,
				SSADD		,
				SSMERGE		,
				SSGETALL	,
				SSGET		,
				SSMGET		,
				SSADDGETALL	,
				SSADDGET	,
				SSADDMGET
			>(pack);
		}
	};

} // namespace


