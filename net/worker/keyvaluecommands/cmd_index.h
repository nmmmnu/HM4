#include "base.h"
//#include "mystring.h"

#include "shared_stoppredicate.h"
#include "shared_accumulateresults.h"

#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::Index{
	namespace impl_{
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
				return extractNth_(N + 2, separator, x);
			};

			return accumulateResults<Out>(maxResults, stop, it, eit, container, proj);
		}



		constexpr bool assertN(int n){
			return n > 0 && n <= 3;
		}

		template<template<int, class, class> class Cmd>
		struct LH{
			template<class Protocol, class DBAdapter>
			using cmd1 = Cmd<1, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd2 = Cmd<2, Protocol, DBAdapter>;

			template<class Protocol, class DBAdapter>
			using cmd3 = Cmd<3, Protocol, DBAdapter>;
		};
	} // namespace impl_



	template<int N, class Protocol, class DBAdapter>
	struct IX_GET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

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
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1get",	"IX1GET"	},
			{	"ix2get",	"IX2GET"	},
			{	"ix3get",	"IX3GET"	}
		};


	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_MGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

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



			auto &container = blob.container();



			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					shared::zsetmulti::get<PN>(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1mget",	"IX1MGET"	},
			{	"ix2mget",	"IX2MGET"	},
			{	"ix3mget",	"IX3MGET"	}
		};
	};



	template<class Protocol, class DBAdapter>
	struct IX_EXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
		//	return std::begin(cmd[N-1]);
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
		//	return std::end(cmd[N-1]);
			return std::end(cmd);
		};

		// IXGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::zsetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		#if 0
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1exists",	"IX1EXISTS"	},
			{	"ix2exists",	"IX2EXISTS"	},
			{	"ix3exists",	"IX3EXISTS"	}
		};
		#endif

		constexpr inline static std::string_view cmd[] = {
				"ix1exists",	"IX1EXISTS",
				"ix2exists",	"IX2EXISTS",
				"ix3exists",	"IX3EXISTS"
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_GETINDEXES : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

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
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1getindexes",	"IX1GETIXES"	},
			{	"ix2getindexes",	"IX2GETIXES"	},
			{	"ix3getindexes",	"IX3GETIXES"	}
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_ADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		// IXADD a keySub0 x0 val0 keySub1 x1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg  = 2;
			auto const vstep = 2 + N;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0){
				switch(N){
				default:
				case 1: return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);
				case 2: return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);
				case 3: return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);
				}
			}

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

					if constexpr(N == 1)
						if (!PN::valid(keyN, keySub, { *(itk + 1) }))
							return e();

					if constexpr(N == 2)
						if (!PN::valid(keyN, keySub, { *(itk + 1), *(itk + 2) }))
							return e();

					if constexpr(N == 3)
						if (!PN::valid(keyN, keySub, { *(itk + 1), *(itk + 2), *(itk + 3) }))
							return e();
				}

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const X = vstep - 1;

				auto const &keySub	= *(itk + 0);
				auto const &value	= *(itk + X);

				{
					if constexpr(N == 1)
						shared::zsetmulti::add<PN>(
								db,
								keyN, keySub, { *(itk + 1) }, value
						);

					if constexpr(N == 2)
						shared::zsetmulti::add<PN>(
								db,
								keyN, keySub, { *(itk + 1), *(itk + 2) }, value
						);

					if constexpr(N == 3)
						shared::zsetmulti::add<PN>(
								db,
								keyN, keySub, { *(itk + 1),  *(itk + 2), *(itk + 3) }, value
						);
				}
			}

			return result.set();
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1add",	"IX1ADD"	},
			{	"ix2add",	"IX2ADD"	},
			{	"ix3add",	"IX3ADD"	}
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_REM : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		// IXDEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<PN>(p, db, result, blob);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][6] = {
			{	"ix1rem",	"IX1REM",	"ix1remove",	"IX1REMOVE",	"ix1del",	"IX1DEL"	},
			{	"ix2rem",	"IX2REM",	"ix2remove",	"IX2REMOVE",	"ix2del",	"IX2DEL"	},
			{	"ix3rem",	"IX3REM",	"ix3remove",	"IX3REMOVE",	"ix3del",	"IX3DEL"	}
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct IX_RANGE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		// IX_RANGE key ABC a  b  c  count from
		// IX_RANGE key ABC a  b  '' count from
		// IX_RANGE key ABC a  '' '' count from
		// IX_RANGE key ABC '' '' '' count from

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			if (p.size() != 1 + 2 + N + 2){
				switch(N){
				default:
				case 1: return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);
				case 2: return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);
				case 3: return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_7);
				}
			}



			auto const keyN     = p[1];
			auto const index    = p[2];

			if (keyN.empty() || index.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const count    = myClamp<uint32_t>(p[2 + N + 1], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
			auto const keyStart = p[2 + N + 2];

			logger<Logger::DEBUG>() << keyN << index << p[2 + 1] << p[2 + 2] << p[2 + 3];

			{
				auto size = [&p](){
					if constexpr(N == 1)
						return p[2 + 1].size();

					if constexpr(N == 2)
						return p[2 + 1].size() + p[2 + 2].size();

					if constexpr(N == 3)
						return p[2 + 1].size() + p[2 + 2].size() + p[2 + 3].size();
				};

				if (!PN::valid(keyN, index, size() ))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			hm4::PairBufferKey bufferKey;

			auto const prefix = [&](){
				if constexpr(N == 1)
					return PN::makeKey(bufferKey, DBAdapter::SEPARATOR, keyN, index, p[2 + 1]);

				if constexpr(N == 2)
					return PN::makeKey(bufferKey, DBAdapter::SEPARATOR, keyN, index, p[2 + 1], p[2 + 2]);

				if constexpr(N == 3)
					return PN::makeKey(bufferKey, DBAdapter::SEPARATOR, keyN, index, p[2 + 1], p[2 + 2], p[2 + 3]);
			}();

			auto const key = keyStart.empty() ? prefix : keyStart;

			auto &container = blob.container();

			accumulateResultsIX_<N, AccumulateOutput::BOTH_WITH_TAIL>(
				count					,
				prefix					,
				db->find(key, std::false_type{})	,
				std::end(*db)				,
				DBAdapter::SEPARATOR[0]			,
				container
			);

			return result.set_container(container);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1range",	"IX1RANGE"	},
			{	"ix2range",	"IX2RANGE"	},
			{	"ix3range",	"IX3RANGE"	}
		};
	};





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "index";

		static void load(RegisterPack &pack){
			using namespace impl_;

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

				IX_EXISTS
			>(pack);
		}
	};



} // namespace

