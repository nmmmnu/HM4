#include "base.h"

#include "shared_stoppredicate.h"
#include "shared_accumulateresults.h"
#include "shared_extractnth.h"

#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::Index{
	namespace impl_{
		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;



		template<int N, AccumulateOutput Out, class It, class Container>
		void accumulateResultsIX_(uint32_t const maxResults, std::string_view const prefix, It it, It eit, char const separator, Container &container){
			StopPrefixPredicate stop{ prefix };

			auto proj = [separator](std::string_view x){
				// a~ABC~a~b~c~d
				return shared::extractnth::extractNth(N + 2 + 1, separator, x);
			};

			return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}



		constexpr bool assertN(int n){
			return n > 0 && n <= 5;
		}

		template<template<int, class, class> class Cmd>
		struct LH{
			template<class Protocol, class DBAdapter>
			using cmd1 = Cmd<1, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd2 = Cmd<2, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd3 = Cmd<3, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd4 = Cmd<4, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd5 = Cmd<5, Protocol, DBAdapter>;

		//	template<class Protocol, class DBAdapter>
		//	using cmd6 = Cmd<6, Protocol, DBAdapter>;
		};
	} // namespace impl_



	template<int N, class Protocol, class DBAdapter>
	struct IX_ADD : BaseCommandRW<Protocol,DBAdapter>{
		IX_ADD() : BaseCommandRW<Protocol,DBAdapter>(name__[N - 1], std::begin(cmd__[N - 1]), std::end(cmd__[N - 1])){}

		// IXADD a keySub0 x0 sort0 val0 keySub1 x1 sort1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			auto const varg  = 2;
			auto const vstep = 1 + N + 1; // subkey + N + sort

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS[4 + N]);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const keySub  = *(itk + 0);

				// auto const keySortSize = shared::sortkey::keySortSize( *(itk + N + 1) );

				auto e = [&result](){
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
				};

				auto _ = [&itk](auto i){
					return *(itk + i);
				};

				if constexpr(N == 1)
					if (!shared::index_token::valid(keyN, keySub, _(1)))
						return e();

				if constexpr(N == 2)
					if (!shared::index_token::valid(keyN, keySub, _(1), _(2)))
						return e();

				if constexpr(N == 3)
					if (!shared::index_token::valid(keyN, keySub, _(1), _(2), _(3)))
						return e();

				if constexpr(N == 4)
					if (!shared::index_token::valid(keyN, keySub, _(1), _(2), _(3), _(4)))
						return e();

				if constexpr(N == 5)
					if (!shared::index_token::valid(keyN, keySub, _(1), _(2), _(3), _(4), _(5)))
						return e();
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				to_string_buffer_t buffer;

				auto const keySub = *(itk + 0);
				auto const keySort = shared::sortkey::makeHashKeySort(keySub, *(itk + N + 1), buffer);

				auto _ = [&itk](auto i){
					return *(itk + i);
				};

				if constexpr(N == 1)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), keySort }, keySub
					);

				if constexpr(N == 2)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), keySort }, keySub
					);

				if constexpr(N == 3)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), keySort }, keySub
					);

				if constexpr(N == 4)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4), keySort }, keySub
					);

				if constexpr(N == 5)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4), _(5), keySort }, keySub
					);

			//	if constexpr(N == 6)
			//		shared::zsetmulti::add<PN>(
			//				db,
			//				keyN, keySub, { _(1), _(2), _(3), _(4), _(5), _(6), keySort }, keySub
			//		);
			}

			return result.set_1();
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view name__[] = {
			"IX1ADD",
			"IX2ADD",
			"IX3ADD",
			"IX4ADD",
			"IX5ADD"
		};

		constexpr inline static std::string_view cmd__[][2] = {
			{ "ix1add", "IX1ADD" },
			{ "ix2add", "IX2ADD" },
			{ "ix3add", "IX3ADD" },
			{ "ix4add", "IX4ADD" },
			{ "ix5add", "IX5ADD" }
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_REM : BaseCommandRW<Protocol,DBAdapter>{
		IX_REM() : BaseCommandRW<Protocol,DBAdapter>(name__[N - 1], std::begin(cmd__[N - 1]), std::end(cmd__[N - 1])){}

		// IXDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<PN>(p, db, result, blob);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view name__[] = {
			"IX1REM",
			"IX2REM",
			"IX3REM",
			"IX4REM",
			"IX5REM"
		};

		constexpr inline static std::string_view cmd__[][2] = {
			{ "ix1rem", "IX1REM" },
			{ "ix2rem", "IX2REM" },
			{ "ix3rem", "IX3REM" },
			{ "ix4rem", "IX4REM" },
			{ "ix5rem", "IX5REM" }
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_GETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		IX_GETINDEXES() : BaseCommandRO<Protocol,DBAdapter>(name__[N - 1], std::begin(cmd__[N - 1]), std::end(cmd__[N - 1])){}

		// IXGETIXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (!shared::index_token::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set_container(
				shared::zsetmulti::getIndexes<PN>(db, keyN, keySub)
			);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view name__[] = {
			"IX1GETINDEXES",
			"IX2GETINDEXES",
			"IX3GETINDEXES",
			"IX4GETINDEXES",
			"IX5GETINDEXES"
		};

		constexpr inline static std::string_view cmd__[][2] = {
			{ "ix1getindexes", "IX1GETINDEXES" },
			{ "ix2getindexes", "IX2GETINDEXES" },
			{ "ix3getindexes", "IX3GETINDEXES" },
			{ "ix4getindexes", "IX4GETINDEXES" },
			{ "ix5getindexes", "IX5GETINDEXES" }
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_RANGE : BaseCommandRO<Protocol,DBAdapter>{
		IX_RANGE() : BaseCommandRO<Protocol,DBAdapter>(name__[N - 1], std::begin(cmd__[N - 1]), std::end(cmd__[N - 1])){}

		// IX_RANGE key ABC a  b  c  count from
		// IX_RANGE key ABC a  b  '' count from
		// IX_RANGE key ABC a  '' '' count from
		// IX_RANGE key ABC '' '' '' count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			using namespace impl_;

			if (p.size() != 1 + 2 + N + 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS[4 + N]);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			auto const count    = myClamp<uint32_t>(p[varg + N + 1], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[varg + N + 2];

			if (!shared::index_token::valid(keyN, index))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			hm4::PairBufferKey bufferKey;

			auto const prefix = [&](){
				auto _ = [&p](uint8_t i){
					return p[varg + i];
				};

				if constexpr(N == 1)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1));

				if constexpr(N == 2)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2));

				if constexpr(N == 3)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2), _(3));

				if constexpr(N == 4)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2), _(3), _(4));

				if constexpr(N == 5)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2), _(3), _(4), _(5));

			//	if constexpr(N == 6)
			//		return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2), _(3), _(4), _(5), _(6));
			}();

			auto const key = keyStart.empty() ? prefix : keyStart;

			logger<Logger::DEBUG>() << "IX_RANGE" << "prefix" << prefix;

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsIX_<N, AccumulateOutput::KEYS_WITH_TAIL>(
				count			,
				prefix			,
				db->find(key)		,
				std::end(*db)		,
				DBAdapter::SEPARATOR[0]	,
				container
			);

			return result.set_container(container);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view name__[] = {
			"IX1RANGE",
			"IX2RANGE",
			"IX3RANGE",
			"IX4RANGE",
			"IX5RANGE"
		};

		constexpr inline static std::string_view cmd__[][2] = {
			{ "ix1range", "IX1RANGE" },
			{ "ix2range", "IX2RANGE" },
			{ "ix3range", "IX3RANGE" },
			{ "ix4range", "IX4RANGE" },
			{ "ix5range", "IX5RANGE" }
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "index";

		static void load(RegisterPack &pack){
			using namespace impl_;

			return registerCommands<Protocol, DBAdapter, RegisterPack,
				LH<IX_GETINDEXES	>::cmd1	,
				LH<IX_ADD		>::cmd1	,
				LH<IX_REM		>::cmd1	,
				LH<IX_RANGE		>::cmd1	,

				LH<IX_GETINDEXES	>::cmd2	,
				LH<IX_ADD		>::cmd2	,
				LH<IX_REM		>::cmd2	,
				LH<IX_RANGE		>::cmd2	,

				LH<IX_GETINDEXES	>::cmd3	,
				LH<IX_ADD		>::cmd3	,
				LH<IX_REM		>::cmd3	,
				LH<IX_RANGE		>::cmd3	,

				LH<IX_GETINDEXES	>::cmd4	,
				LH<IX_ADD		>::cmd4	,
				LH<IX_REM		>::cmd4	,
				LH<IX_RANGE		>::cmd4	,

				LH<IX_GETINDEXES	>::cmd5	,
				LH<IX_ADD		>::cmd5	,
				LH<IX_REM		>::cmd5	,
				LH<IX_RANGE		>::cmd5

			//	LH<IX_GETINDEXES	>::cmd6	,
			//	LH<IX_ADD		>::cmd6	,
			//	LH<IX_REM		>::cmd6	,
			//	LH<IX_RANGE		>::cmd6
			>(pack);
		}
	};



} // namespace

