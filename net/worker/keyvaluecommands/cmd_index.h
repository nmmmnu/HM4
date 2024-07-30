#include "base.h"
//#include "mystring.h"

//#include "shared_stoppredicate.h"
//#include "shared_iterations.h"
#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::Index{
	namespace impl_{
		constexpr bool assertN(int n){
			return n > 0 && n <= 3;
		}
	}



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

	template<class Protocol, class DBAdapter>	using IX1GET = IX_GET<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2GET = IX_GET<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3GET = IX_GET<3, Protocol, DBAdapter>;



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

	template<class Protocol, class DBAdapter>	using IX1MGET = IX_MGET<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2MGET = IX_MGET<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3MGET = IX_MGET<3, Protocol, DBAdapter>;



	template<int N, class Protocol, class DBAdapter>
	struct IX_EXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		// IXGET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return shared::zsetmulti::cmdProcessExists<PN>(p, db, result, blob);
		}

	private:
		static_assert(impl_::assertN(N));

		using PN = shared::zsetmulti::Permutation<N>;

		constexpr inline static std::string_view cmd[][2] = {
			{	"ix1exists",	"IX1EXISTS"	},
			{	"ix2exists",	"IX2EXISTS"	},
			{	"ix3exists",	"IX3EXISTS"	}
		};
	};

	template<class Protocol, class DBAdapter>	using IX1EXISTS = IX_EXISTS<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2EXISTS = IX_EXISTS<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3EXISTS = IX_EXISTS<3, Protocol, DBAdapter>;



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

	template<class Protocol, class DBAdapter>	using IX1GETINDEXES = IX_GETINDEXES<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2GETINDEXES = IX_GETINDEXES<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3GETINDEXES = IX_GETINDEXES<3, Protocol, DBAdapter>;



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
			static_assert(N >= 1 && N <= 3);

			auto const varg  = 2;
			auto const vstep = 2 + N;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0){
				if constexpr(N == 1)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

				if constexpr(N == 2)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

				if constexpr(N == 3)
					return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_6);
			}

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const X = vstep - 1;

				auto const &keySub	= *(itk + 0);
				auto const &value	= *(itk + X);

				if constexpr(N == 1)
					if (!PN::valid(keyN, keySub, { *(itk + 1) }))
						return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if constexpr(N == 2)
					if (!PN::valid(keyN, keySub, { *(itk + 1), *(itk + 2) }))
						return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if constexpr(N == 3)
					if (!PN::valid(keyN, keySub, { *(itk + 1), *(itk + 2), *(itk + 3) }))
						return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const X = vstep - 1;

				auto const &keySub	= *(itk + 0);
				auto const &value	= *(itk + X);

				if constexpr(N == 1){
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { *(itk + 1) }, value
					);
				}

				if constexpr(N == 2){
					shared::zsetmulti::add<PN>(
							db,
							keyN, keySub, { *(itk + 1), *(itk + 2) }, value
					);
				}

				if constexpr(N == 3){
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

	template<class Protocol, class DBAdapter>	using IX1ADD = IX_ADD<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2ADD = IX_ADD<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3ADD = IX_ADD<3, Protocol, DBAdapter>;



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

	template<class Protocol, class DBAdapter>	using IX1REM = IX_REM<1, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX2REM = IX_REM<2, Protocol, DBAdapter>;
	template<class Protocol, class DBAdapter>	using IX3REM = IX_REM<3, Protocol, DBAdapter>;





	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "index";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				IX1GET		,
				IX1MGET		,
				IX1EXISTS	,
				IX1GETINDEXES	,
				IX1ADD		,
				IX1REM		,

				IX2GET		,
				IX2MGET		,
				IX2EXISTS	,
				IX2GETINDEXES	,
				IX2ADD		,
				IX2REM		,

				IX3GET		,
				IX3MGET		,
				IX3EXISTS	,
				IX3GETINDEXES	,
				IX3ADD		,
				IX3REM
			>(pack);
		}
	};



} // namespace

