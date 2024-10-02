#include "base.h"
#include "tdigest.h"
#include "pair_vfactory.h"
#include "to_fp.h"

namespace net::worker::commands::TDigest{
	namespace td_impl_{

		using Pair = hm4::Pair;

		auto const MIN_SIZE = 16;
		auto const MAX_SIZE = 20'000;

		static_assert(RawTDigest::bytes(MAX_SIZE) <= hm4::PairConf::MAX_VAL_SIZE);


		std::string_view formatLine(to_string_buffer_t &buffer, double d){
			constexpr static std::string_view fmt_mask = "{:+015.15f}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, d);

			if (result.out == std::end(buffer))
				return {};
			else
				return { buffer.data(), result.size };
		};


	} // namespace td_impl_



	template<class Protocol, class DBAdapter>
	struct TDRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdreserve",	"TDRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		}

		const std::string_view *end()   const final{
			return std::end(cmd);
		}

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

			RawTDigest td{ c };

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes());

			using MyTDADD_Factory = TDADD_Factory<ParamContainer::iterator>;

			MyTDADD_Factory factory{ key, pair, td, d, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, *db, factory);

			return result.set_1();
		}

	private:
		template<typename It>
		struct TDADD_Factory : hm4::PairFactory::IFactoryAction<1, 1, TDADD_Factory<It> >{
			using Pair   = hm4::Pair;
			using Base   = hm4::PairFactory::IFactoryAction<1, 1, TDADD_Factory<It> >;

			constexpr TDADD_Factory(std::string_view const key, const Pair *pair, RawTDigest &tdigest, double delta, It begin, It end) :
							Base::IFactoryAction	(key, tdigest.bytes(), pair),
							tdigest			(tdigest	),
							delta			(delta		),
							begin			(begin		),
							end			(end		){}

			void action(Pair *pair) const{
				using namespace td_impl_;

				auto const compression = RawTDigest::Compression::AGGRESSIVE;

				auto *data = reinterpret_cast<RawTDigest::TDigest *>(pair->getValC());

				for(auto it = begin; it != end; ++it){
					auto const val = to_double_def(*it);
					tdigest.add<compression>(data, delta, val);
				}
			}

		private:
			RawTDigest	&tdigest;
			double		delta;
			It		begin;
			It		end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdadd",	"TDADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDPERCENTILE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// TDADD key capacity percentile [percentile]
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
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

			if (const auto *pair = hm4::getPairPtrWithSize(*db, key, td.bytes()); pair == nullptr){
				auto &container = blob.container();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
					container.push_back("0.0");

				return result.set_container(container);
			}else{
				const auto *data = reinterpret_cast<const RawTDigest::TDigest *>(pair->getValC());


				auto &container  = blob.container();
				auto &bcontainer = blob.bcontainer();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const p = std::clamp<double>(to_double_def(*itk), 0.0, 1.0);
					auto const r = td.percentile(data, p);

					bcontainer.push_back();

					auto const line = formatLine(bcontainer.back(), r);

					container.push_back(line);
				}

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdpercentile",	"TDPERCENTILE",
			"thquantile",	"TDQUANTILE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDMIN : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// TDADD key capacity
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
				const auto *data = reinterpret_cast<const RawTDigest::TDigest *>(pair->getValC());

				auto const r = td.min(data);

				to_string_buffer_t buffer;
				auto const line = formatLine(buffer, r);

				return result.set(line);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdmin",	"TDMIN"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDMAX : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// TDADD key capacity
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
				const auto *data = reinterpret_cast<const RawTDigest::TDigest *>(pair->getValC());

				auto const r = td.max(data);

				to_string_buffer_t buffer;
				auto const line = formatLine(buffer, r);

				return result.set(line);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdmax",	"TDMAX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDSIZE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// TDADD key capacity
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
				const auto *data = reinterpret_cast<const RawTDigest::TDigest *>(pair->getValC());

				return result.set(td.weight(data));
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdsize",	"TDSIZE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct TDCENTROIDCOUNT : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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
				return result.set(uint64_t{0});
			}else{
				const auto *data = reinterpret_cast<const RawTDigest::TDigest *>(pair->getValC());

				return result.set(td.size(data));
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"tdcentroidcount",	"TDCENTROIDCOUNT"	,
			"tdcentroidscount",	"TDCENTROIDSCOUNT"	,
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "tdigest";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				TDRESERVE	,
				TDADD		,
				TDPERCENTILE	,
				TDMIN		,
				TDMAX		,
				TDSIZE		,
				TDCENTROIDCOUNT
			>(pack);
		}
	};



} // namespace



