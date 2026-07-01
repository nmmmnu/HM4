#include "base.h"

#include "shared_stoppredicate.h"
#include "shared_accumulateresults.h"

#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::Index{
	namespace index_impl_{
		using namespace net::worker::shared::accumulate_results;
		using namespace net::worker::shared::config;



		inline std::string_view extractNth_(size_t const nth, char const separator, std::string_view const s){
			size_t count = 0;

			for (size_t i = 0; i < s.size(); ++i)
				if (s[i] == separator)
					if (++count; count == nth)
						return s.substr(i + 1);

			return "INVALID_DATA";
		}



		template<int N, AccumulateOutput Out, class It, class Container>
		void accumulateResultsIX_(uint32_t const maxResults, std::string_view const prefix, It it, It eit, char const separator, Container &container){
			StopPrefixPredicate stop{ prefix };

			auto proj = [separator](std::string_view x){
				// a~ABC~a~b~c~d
				return extractNth_(N + 2 + 1, separator, x);
			};

			return sharedAccumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}



		constexpr bool assertN(int n){
			return n > 0 && n <= 6;
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

			template<class Protocol, class DBAdapter>
			using cmd6 = Cmd<6, Protocol, DBAdapter>;
		};
	} // namespace index_impl_



	template<int N, class Protocol, class DBAdapter>
	struct IX_GET : BaseCommandRO<Protocol,DBAdapter>{
		IX_GET() : BaseCommandRO<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IXGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				shared::zsetmulti::get<PN>(db, keyN, keySub)
			);
		}

	private:
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1GET";
			if constexpr(N == 2) return "IX2GET";
			if constexpr(N == 3) return "IX3GET";
			if constexpr(N == 4) return "IX4GET";
			if constexpr(N == 5) return "IX5GET";
			if constexpr(N == 6) return "IX6GET";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1get", "IX1GET" };
			if constexpr(N == 2) return { "ix2get", "IX2GET" };
			if constexpr(N == 3) return { "ix3get", "IX3GET" };
			if constexpr(N == 4) return { "ix4get", "IX4GET" };
			if constexpr(N == 5) return { "ix5get", "IX5GET" };
			if constexpr(N == 6) return { "ix6get", "IX6GET" };
		}
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_MGET : BaseCommandRO<Protocol,DBAdapter>{
		IX_MGET() : BaseCommandRO<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IXGET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				if (keySub.empty())
					return result.set_error(ResultErrorMessages::EMPTY_KEY);

				if (!PN::valid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}



			auto &container = blob.construct<OutputBlob::Container>();



			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					shared::zsetmulti::get<PN>(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

	private:
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1MGET";
			if constexpr(N == 2) return "IX2MGET";
			if constexpr(N == 3) return "IX3MGET";
			if constexpr(N == 4) return "IX4MGET";
			if constexpr(N == 5) return "IX5MGET";
			if constexpr(N == 6) return "IX6MGET";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1mget", "IX1MGET" };
			if constexpr(N == 2) return { "ix2mget", "IX2MGET" };
			if constexpr(N == 3) return { "ix3mget", "IX3MGET" };
			if constexpr(N == 4) return { "ix4mget", "IX4MGET" };
			if constexpr(N == 5) return { "ix5mget", "IX5MGET" };
			if constexpr(N == 6) return { "ix6mget", "IX6MGET" };
		}
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_GETINDEXES : BaseCommandRO<Protocol,DBAdapter>{
		IX_GETINDEXES() : BaseCommandRO<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IXGETIXES key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!PN::valid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set_container(
				shared::zsetmulti::getIndexes<PN>(db, keyN, keySub)
			);
		}

	private:
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1GETINDEXES";
			if constexpr(N == 2) return "IX2GETINDEXES";
			if constexpr(N == 3) return "IX3GETINDEXES";
			if constexpr(N == 4) return "IX4GETINDEXES";
			if constexpr(N == 5) return "IX5GETINDEXES";
			if constexpr(N == 6) return "IX6GETINDEXES";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1getindexes", "IX1GETINDEXES" };
			if constexpr(N == 2) return { "ix2getindexes", "IX2GETINDEXES" };
			if constexpr(N == 3) return { "ix3getindexes", "IX3GETINDEXES" };
			if constexpr(N == 4) return { "ix4getindexes", "IX4GETINDEXES" };
			if constexpr(N == 5) return { "ix5getindexes", "IX5GETINDEXES" };
			if constexpr(N == 6) return { "ix6getindexes", "IX6GETINDEXES" };
		}
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_ADD : BaseCommandRW<Protocol,DBAdapter>{
		IX_ADD() : BaseCommandRW<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IXADD a keySub0 x0 sort0 val0 keySub1 x1 sort1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg  = 2;
			auto const vstep = 2 + N + 1; // N + 1 for sort

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS[4 + N]);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const X = vstep - 1;

				auto const &keySub	= *(itk + 0);
				auto const &value	= *(itk + X);

				{
					auto e = [&result](){
						return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
					};

					auto _ = [&itk](auto i){
						return *(itk + i);
					};

					if constexpr(N == 1)
						if (!PN::valid(keyN, keySub, { _(1), _(2) }))
							return e();

					if constexpr(N == 2)
						if (!PN::valid(keyN, keySub, { _(1), _(2), _(3) }))
							return e();

					if constexpr(N == 3)
						if (!PN::valid(keyN, keySub, { _(1), _(2), _(3), _(4) }))
							return e();

					if constexpr(N == 4)
						if (!PN::valid(keyN, keySub, { _(1), _(2), _(3), _(4), _(5) }))
							return e();

					if constexpr(N == 5)
						if (!PN::valid(keyN, keySub, { _(1), _(2), _(3), _(4), _(5), _(6) }))
							return e();

					if constexpr(N == 6)
						if (!PN::valid(keyN, keySub, { _(1), _(2), _(3), _(4), _(5), _(6), _(7) }))
							return e();
				}

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const X = vstep - 1;

				auto const &keySub	= *(itk + 0);
				auto const &value	= *(itk + X);

				auto _ = [&itk](auto i){
					return *(itk + i);
				};

				if constexpr(N == 1)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2) }, value
					);

				if constexpr(N == 2)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3) }, value
					);

				if constexpr(N == 3)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4) }, value
					);

				if constexpr(N == 4)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4), _(5) }, value
					);

				if constexpr(N == 5)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4), _(5), _(6) }, value
					);

				if constexpr(N == 6)
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { _(1), _(2), _(3), _(4), _(5), _(6), _(7) }, value
					);
			}

			return result.set();
		}

	private:
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1ADD";
			if constexpr(N == 2) return "IX2ADD";
			if constexpr(N == 3) return "IX3ADD";
			if constexpr(N == 4) return "IX4ADD";
			if constexpr(N == 5) return "IX5ADD";
			if constexpr(N == 6) return "IX6ADD";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1add", "IX1ADD" };
			if constexpr(N == 2) return { "ix2add", "IX2ADD" };
			if constexpr(N == 3) return { "ix3add", "IX3ADD" };
			if constexpr(N == 4) return { "ix4add", "IX4ADD" };
			if constexpr(N == 5) return { "ix5add", "IX5ADD" };
			if constexpr(N == 6) return { "ix6add", "IX6ADD" };
		}
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_REM : BaseCommandRW<Protocol,DBAdapter>{
		IX_REM() : BaseCommandRW<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IXDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<PN>(p, db, result, blob);
		}

	private:
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1REM";
			if constexpr(N == 2) return "IX2REM";
			if constexpr(N == 3) return "IX3REM";
			if constexpr(N == 4) return "IX4REM";
			if constexpr(N == 5) return "IX5REM";
			if constexpr(N == 6) return "IX6REM";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1rem", "IX1REM", "ix1remove", "IX1REMOVE", "ix1del", "IX1DEL" };
			if constexpr(N == 2) return { "ix2rem", "IX2REM", "ix2remove", "IX2REMOVE", "ix2del", "IX2DEL" };
			if constexpr(N == 3) return { "ix3rem", "IX3REM", "ix3remove", "IX3REMOVE", "ix3del", "IX3DEL" };
			if constexpr(N == 4) return { "ix4rem", "IX4REM", "ix4remove", "IX4REMOVE", "ix4del", "IX4DEL" };
			if constexpr(N == 5) return { "ix5rem", "IX5REM", "ix5remove", "IX5REMOVE", "ix5del", "IX5DEL" };
			if constexpr(N == 6) return { "ix6rem", "IX6REM", "ix6remove", "IX6REMOVE", "ix6del", "IX6DEL" };
		}
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_RANGE : BaseCommandRO<Protocol,DBAdapter>{
		IX_RANGE() : BaseCommandRO<Protocol,DBAdapter>(getName__(), getCmd__()){}

		// IX_RANGE key ABC a  b  c  count from
		// IX_RANGE key ABC a  b  '' count from
		// IX_RANGE key ABC a  '' '' count from
		// IX_RANGE key ABC '' '' '' count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace index_impl_;

			if (p.size() != 1 + 2 + N + 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS[4 + N]);

			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const varg = 2;

			auto const count    = myClamp<uint32_t>(p[varg + N + 1], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[varg + N + 2];

			{
				auto size = [&p](){
					#if 0
					size_t size = 0;
					for(size_t i = 1; i <= N; ++i)
						size += p[varg + i].size();

					return size;
					#endif

					// intent is more important

					auto _ = [&p](uint8_t i){
						return p[varg + i].size();
					};

					if constexpr(N == 1)
						return _(1);

					if constexpr(N == 2)
						return _(1) + _(2);

					if constexpr(N == 3)
						return _(1) + _(2) + _(3);

					if constexpr(N == 4)
						return _(1) + _(2) + _(3) + _(4);

					if constexpr(N == 5)
						return _(1) + _(2) + _(3) + _(4) + _(5);

					if constexpr(N == 6)
						return _(1) + _(2) + _(3) + _(4) + _(5) + _(6);
				};

				if (!PN::valid(keyN, index, size() ))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

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

				if constexpr(N == 6)
					return PN::makeKeyRange(bufferKey, DBAdapter::SEPARATOR, keyN, index, _(1), _(2), _(3), _(4), _(5), _(6));
			}();

			auto const key = keyStart.empty() ? prefix : keyStart;

			logger<Logger::DEBUG>() << "IX_RANGE" << "prefix" << prefix;

			auto &container = blob.construct<OutputBlob::Container>();

			accumulateResultsIX_<N, AccumulateOutput::BOTH_WITH_TAIL>(
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
		static_assert(index_impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr static const char *getName__(){
			if constexpr(N == 1) return "IX1RANGE";
			if constexpr(N == 2) return "IX2RANGE";
			if constexpr(N == 3) return "IX3RANGE";
			if constexpr(N == 4) return "IX4RANGE";
			if constexpr(N == 5) return "IX5RANGE";
			if constexpr(N == 6) return "IX6RANGE";
		}

		constexpr static CommandAliasesContainer getCmd__(){
			if constexpr(N == 1) return { "ix1range", "IX1RANGE" };
			if constexpr(N == 2) return { "ix2range", "IX2RANGE" };
			if constexpr(N == 3) return { "ix3range", "IX3RANGE" };
			if constexpr(N == 4) return { "ix4range", "IX4RANGE" };
			if constexpr(N == 5) return { "ix5range", "IX5RANGE" };
			if constexpr(N == 6) return { "ix6range", "IX6RANGE" };
		}
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "index";

		static void load(RegisterPack &pack){
			using namespace index_impl_;

			return registerCommands<Protocol, DBAdapter, RegisterPack,
				LH<IX_GET		>::cmd1	,
				LH<IX_MGET		>::cmd1	,
				LH<IX_GETINDEXES	>::cmd1	,
				LH<IX_ADD		>::cmd1	,
				LH<IX_REM		>::cmd1	,
				LH<IX_RANGE		>::cmd1	,

				LH<IX_GET		>::cmd2	,
				LH<IX_MGET		>::cmd2	,
				LH<IX_GETINDEXES	>::cmd2	,
				LH<IX_ADD		>::cmd2	,
				LH<IX_REM		>::cmd2	,
				LH<IX_RANGE		>::cmd2	,

				LH<IX_GET		>::cmd3	,
				LH<IX_MGET		>::cmd3	,
				LH<IX_GETINDEXES	>::cmd3	,
				LH<IX_ADD		>::cmd3	,
				LH<IX_REM		>::cmd3	,
				LH<IX_RANGE		>::cmd3	,

				LH<IX_GET		>::cmd4	,
				LH<IX_MGET		>::cmd4	,
				LH<IX_GETINDEXES	>::cmd4	,
				LH<IX_ADD		>::cmd4	,
				LH<IX_REM		>::cmd4	,
				LH<IX_RANGE		>::cmd4	,

				LH<IX_GET		>::cmd5	,
				LH<IX_MGET		>::cmd5	,
				LH<IX_GETINDEXES	>::cmd5	,
				LH<IX_ADD		>::cmd5	,
				LH<IX_REM		>::cmd5	,
				LH<IX_RANGE		>::cmd5	,

				LH<IX_GET		>::cmd6	,
				LH<IX_MGET		>::cmd6	,
				LH<IX_GETINDEXES	>::cmd6	,
				LH<IX_ADD		>::cmd6	,
				LH<IX_REM		>::cmd6	,
				LH<IX_RANGE		>::cmd6
			>(pack);
		}
	};



} // namespace

