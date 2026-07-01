#include "base.h"
#include "tdigest.h"
#include "pair_vfactory.h"
#include "to_fp.h"
#include "shared_hint.h"

namespace net::worker::commands::TDigest{
	namespace td_impl_{

		using Pair = hm4::Pair;

		constexpr auto MIN_SIZE		= 16;
		constexpr auto MAX_SIZE		= RawTDigest::maxCapacity(hm4::PairConf::MAX_VAL_SIZE);

		constexpr auto MIN_WEIGHT	= 1;
		constexpr auto MAX_WEIGHT	= 0xFFFF;

		static_assert(RawTDigest::bytes(MAX_SIZE) <= hm4::PairConf::MAX_VAL_SIZE);



		std::string_view formatLine(double d, to_string_buffer_t &buffer){
			constexpr static std::string_view fmt_mask = "{:+015.15f}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		};



		struct TDItem{
			double_t	value;
			uint64_t	weight;
		};

		using TDItemVector = StaticVector<TDItem, OutputBlob::ParamContainerSize>;



		struct TDADD_Factory : hm4::PairFactory::IFactoryAction<1, 1, TDADD_Factory>{
			using Pair	= hm4::Pair;
			using Base	= hm4::PairFactory::IFactoryAction<1, 1, TDADD_Factory>;

			using It	= TDItemVector::const_iterator;

			constexpr TDADD_Factory(std::string_view const key, const Pair *pair, RawTDigest &tdigest, double delta, It begin, It end) :
							Base::IFactoryAction	(key, tdigest.bytes(), pair),
							tdigest			(tdigest	),
							delta			(delta		),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair) const{
				using namespace td_impl_;

				auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				for(auto it = begin; it != end; ++it)
					tdigest.add<compression>(data, delta, it->value, it->weight);
			}

		private:
			constexpr static auto compression = RawTDigest::Compression::AGGRESSIVE;

		private:
			RawTDigest	&tdigest;
			double		delta;
			It		begin;
			It		end;
		};



		struct TDPair{
			const Pair	*pair;
			uint64_t	size;
		};

		using TDPairVector = StaticVector<TDPair, OutputBlob::ParamContainerSize>;



		struct TDMERGE_Factory : hm4::PairFactory::IFactoryAction<1, 1, TDMERGE_Factory>{
			using Pair	= hm4::Pair;
			using Base	= hm4::PairFactory::IFactoryAction<1, 1, TDMERGE_Factory>;

			using It	= TDPairVector::const_iterator;

			constexpr TDMERGE_Factory(std::string_view const key, const Pair *pair, RawTDigest &tdigest, double delta, It begin, It end) :
							Base::IFactoryAction	(key, tdigest.bytes(), pair),
							tdigest			(tdigest	),
							delta			(delta		),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair) const{
				using namespace td_impl_;

				auto const compression = RawTDigest::Compression::AGGRESSIVE;

				auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				for(auto it = begin; it != end; ++it){
					const auto *src_data = hm4::getValAs<RawTDigest::TDigest>(it->pair);

					RawTDigest src_tdigest{ it->size };

					merge<compression>(tdigest, data, delta, src_tdigest, src_data);
				}
			}

		private:
			RawTDigest	&tdigest;
			double		delta;
			It		begin;
			It		end;
		};

	} // namespace td_impl_



	template<class Protocol, class DBAdapter>
	struct TDRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		TDRESERVE() : BaseCommandRW<Protocol,DBAdapter>("TDRESERVE", {
			"tdreserve",	"TDRESERVE"
		}){}

		// TDRESERVE key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (td.bytes() > MAX_SIZE){
				// emit an error
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);
			}

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, td.bytes());

			return result.set_1();
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDADD : BaseCommandRW<Protocol,DBAdapter>{
		TDADD() : BaseCommandRW<Protocol,DBAdapter>("TDADD", {
			"tdadd",	"TDADD"
		}){}

		// TDADD key capacity delta value [value]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);
			auto const d = std::clamp<double>(to_double_def(p[3]), 0.0, 1.0);

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			TDItemVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const value  = to_double_def(*itk);
				auto const weight = MIN_WEIGHT;

				container.push_back( TDItem{ value, weight } );
			}

			RawTDigest td{ c };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes());

			TDADD_Factory factory{ key, pair, td, d, std::begin(container), std::end(container) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDADDWEIGHT : BaseCommandRW<Protocol,DBAdapter>{
		TDADDWEIGHT() : BaseCommandRW<Protocol,DBAdapter>("TDADDWEIGHT", {
			"tdaddweight",	"TDADDWEIGHT"
		}){}

		// TDADDWEIGHT key capacity delta value weight [value weight]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			auto const varg = 4;

			if (p.size() < 6 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);
			auto const d = std::clamp<double>(to_double_def(p[3]), 0.0, 1.0);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			TDItemVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const value  = to_double_def(*itk);
				auto const weight = std::clamp<uint64_t>(from_string<uint64_t>(*std::next(itk)), MIN_WEIGHT, MAX_WEIGHT);

				container.push_back( TDItem{ value, weight } );
			}

			RawTDigest td{ c };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes());

			TDADD_Factory factory{ key, pair, td, d, std::begin(container), std::end(container) };

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDMERGE : BaseCommandRW<Protocol,DBAdapter>{
		TDMERGE() : BaseCommandRW<Protocol,DBAdapter>("TDMERGE", {
			"tdmerge",	"TDMERGE"
		}){}

		// TDMERGE key capacity delta src_key [src_key]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);
			auto const d = std::clamp<double>(to_double_def(p[3]), 0.0, 1.0);

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &src = *itk; !hm4::Pair::isKeyValid(src))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			RawTDigest td{ c };

			TDPairVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &src_key = *itk;

				// prevent merge with itself.
				if (src_key == key)
					continue;

				auto const src_c = c;

				RawTDigest &td_src = td;

				if (const auto *pair = hm4::getPairPtrWithSize(*db, src_key, td_src.bytes()); pair)
					container.push_back( TDPair{ pair, src_c } );

			}

			if (container.empty())
				return result.set_1();

			const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes());

			TDMERGE_Factory factory{ key, pair, td, d, std::begin(container), std::end(container) };

			// This is fine, because flush list give guarantees now.

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	
		constexpr static auto compression = RawTDigest::Compression::AGGRESSIVE;

	};



	template<class Protocol, class DBAdapter>
	struct TDMERGECAPACITY : BaseCommandRW<Protocol,DBAdapter>{
		TDMERGECAPACITY() : BaseCommandRW<Protocol,DBAdapter>("TDMERGECAPACITY", {
			"tdmergecapacity",	"TDMERGECAPACITY"	,
			"tdmergecap",		"TDMERGECAP"
		}){}

		// TDMERGE key capacity delta src_key [src_key src_capacity]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() < 6)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);
			auto const d = std::clamp<double>(to_double_def(p[3]), 0.0, 1.0);

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &src = *itk; !hm4::Pair::isKeyValid(src))
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

			TDPairVector container;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &src_key = *itk;

				// prevent merge with itself.
				if (src_key == key)
					continue;

				auto const src_c = std::clamp<uint64_t>(from_string<uint64_t>(*std::next(itk)), MIN_SIZE, MAX_SIZE);

				RawTDigest td_src{ src_c };

				if (const auto *pair = hm4::getPairPtrWithSize(*db, src_key, td_src.bytes()); pair)
					container.push_back( TDPair{ pair, src_c } );
			}

			if (container.empty())
				return result.set_1();

			RawTDigest td{ c };

			const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes());

			TDMERGE_Factory factory{ key, pair, td, d, std::begin(container), std::end(container) };

			// This is fine, because flush list give guarantees now.

			insertHintVFactory(*db, pair, factory);

			return result.set_1();
		}

	
		constexpr static auto compression = RawTDigest::Compression::AGGRESSIVE;

	};



	template<class Protocol, class DBAdapter>
	struct TDPERCENTILE : BaseCommandRO<Protocol,DBAdapter>{
		TDPERCENTILE() : BaseCommandRO<Protocol,DBAdapter>("TDPERCENTILE", {
			"tdpercentile",	"TDPERCENTILE",
			"thquantile",	"TDQUANTILE"
		}){}

		// TDPERCENTILE key capacity percentile
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			const auto &val = p[3];

			if (val.empty())
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				to_string_buffer_t buffer;

				auto const percentile       = std::clamp<double>(to_double_def(val), 0.0, 1.0);
				auto const percentileResult = td.percentile(data, percentile);

				auto const line             = formatLine(percentileResult, buffer);

				return result.set(line);
			}
		}

	
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	};



	template<class Protocol, class DBAdapter>
	struct TDMPERCENTILE : BaseCommandRO<Protocol,DBAdapter>{
		TDMPERCENTILE() : BaseCommandRO<Protocol,DBAdapter>("TDMPERCENTILE", {
			"tdmpercentile",	"TDMPERCENTILE",
			"thmquantile",		"TDMQUANTILE"
		}){}

		// TDMPERCENTILE key capacity percentile [percentile]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() < 4)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			auto const varg = 3;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			Container container;

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0.0");

				return result.set_container(container);
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				BContainer bcontainer;

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const p = std::clamp<double>(to_double_def(*itk), 0.0, 1.0);
					auto const r = td.percentile(data, p);

					bcontainer.push_back();

					auto const line = formatLine(r, bcontainer.back());

					container.push_back(line);
				}

				return result.set_container(container);
			}
		}

	
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	};



	template<class Protocol, class DBAdapter>
	struct TDMEDIAN : BaseCommandRO<Protocol,DBAdapter>{
		TDMEDIAN() : BaseCommandRO<Protocol,DBAdapter>("TDMEDIAN", {
			"tdmedian",	"TDMEDIAN"
		}){}

		// TDMEDIAN key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				auto const percentile       = 0.5;
				auto const percentileResult = td.percentile(data, percentile);

				to_string_buffer_t buffer;

				auto const line             = formatLine(percentileResult, buffer);

				return result.set(line);
			}
		}

	
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	};



	template<class Protocol, class DBAdapter>
	struct TDTRIMMEDMEAN : BaseCommandRO<Protocol,DBAdapter>{
		TDTRIMMEDMEAN() : BaseCommandRO<Protocol,DBAdapter>("TDTRIMMEDMEAN", {
			"tdtrimmedmean",	"TDTRIMMEDMEAN"
		}){}

		// TDTRIMMEDMEAN key capacity from_percentile to_percentile
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			auto const p1 = std::clamp<double>(to_double_def(p[3]), 0.0, 1.0);
			auto const p2 = std::clamp<double>(to_double_def(p[4]), 0.0, 1.0);

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				auto const trimmedMeanResult = td.trimmedMean(data, p1, p2);

				to_string_buffer_t buffer;

				auto const line = formatLine(trimmedMeanResult, buffer);

				return result.set(line);
			}
		}

	
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	};



	template<class Protocol, class DBAdapter>
	struct TDMEAN : BaseCommandRO<Protocol,DBAdapter>{
		TDMEAN() : BaseCommandRO<Protocol,DBAdapter>("TDMEAN", {
			"tdmean",	"TDMEAN"
		}){}

		// TDMEAN key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				auto const meanResult = td.mean(data);

				to_string_buffer_t buffer;

				auto const line = formatLine(meanResult, buffer);

				return result.set(line);
			}
		}

	
		using Container  = StaticVector<std::string_view,	OutputBlob::ParamContainerSize>;
		using BContainer = StaticVector<to_string_buffer_t,	OutputBlob::ParamContainerSize>;

	};



	template<class Protocol, class DBAdapter>
	struct TDMIN : BaseCommandRO<Protocol,DBAdapter>{
		TDMIN() : BaseCommandRO<Protocol,DBAdapter>("TDMIN", {
			"tdmin",	"TDMIN"
		}){}

		// TDMIN key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				auto const r = td.min(data);

				to_string_buffer_t buffer;
				auto const line = formatLine(r, buffer);

				return result.set(line);
			}
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDMAX : BaseCommandRO<Protocol,DBAdapter>{
		TDMAX() : BaseCommandRO<Protocol,DBAdapter>("TDMAX", {
			"tdmax",	"TDMAX"
		}){}

		// TDMAX key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set("0.0");
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				auto const r = td.max(data);

				to_string_buffer_t buffer;
				auto const line = formatLine(r, buffer);

				return result.set(line);
			}
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDSIZE : BaseCommandRO<Protocol,DBAdapter>{
		TDSIZE() : BaseCommandRO<Protocol,DBAdapter>("TDSIZE", {
			"tdsize",	"TDSIZE"
		}){}

		// TDSIZE key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				return result.set(uint64_t{0});
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				return result.set(td.weight(data));
			}
		}

	};



	template<class Protocol, class DBAdapter>
	struct TDINFO : BaseCommandRO<Protocol,DBAdapter>{
		TDINFO() : BaseCommandRO<Protocol,DBAdapter>("TDINFO", {
			"tdinfo",	"TDINFO"
		}){}

		// TDCENTROIDCOUNT key capacity
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace td_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const c = std::clamp<uint64_t>(from_string<uint64_t>(p[2]), MIN_SIZE, MAX_SIZE);

			RawTDigest td{ c };


			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				std::array<std::string_view, 2> const container{
					"state"	,	"invalid"
				};

				return result.set_container(container);
			}else{
				const auto *data = hm4::getValAs<RawTDigest::TDigest>(pair);

				to_string_buffer_t buffer[5];

				std::array<std::string_view, 12> const container{
					"state"		,	"ok",
					"capacity"	,	to_string (c			, buffer[0]),
					"size"		,	to_string (td.weight(data)	, buffer[1]),
					"min"		,	formatLine(td.min(data)		, buffer[2]),
					"max"		,	formatLine(td.max(data)		, buffer[3]),
					"nodes"		,	to_string (td.size(data)	, buffer[4])
				};

				return result.set_container(container);
			}
		}

	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "tdigest";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				TDRESERVE	,
				TDADD		,
				TDADDWEIGHT	,
				TDMERGE		,
				TDMERGECAPACITY	,
				TDPERCENTILE	,
				TDMPERCENTILE	,
				TDMEDIAN	,
				TDTRIMMEDMEAN	,
				TDMEAN		,
				TDMIN		,
				TDMAX		,
				TDSIZE		,
				TDINFO
			>(pack);
		}
	};



} // namespace



