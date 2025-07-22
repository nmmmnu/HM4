#include "base.h"

#include "shared_random.h"

namespace net::worker::commands::Random{

	namespace random_impl_{
		template<int N>
		struct RT;

		template<> struct RT<1>{ using type = uint64_t;	};
		template<> struct RT<2>{ using type = uint32_t;	};
		template<> struct RT<3>{ using type = uint16_t;	};
		template<> struct RT<4>{ using type = uint8_t;	};

		template<int N>
		using RTT = typename RT<N>::type;

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
		};
	}

	template<int N, class Protocol, class DBAdapter>
	struct RANDOM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		// RANDOM / RANDOM 0

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 1 && p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_01);

			auto const index = p.size() == 1 ?
						0 :
						from_string<size_t>(p[1])
			;

			using namespace random_impl_;

			uint64_t const r = shared::random::get<RTT<N> >(index);

			return result.set(r);
		}

	private:
		constexpr inline static std::string_view cmd[][6] = {
			{	"random64",	"RANDOM64",	"rand64",	"RAND64"	},
			{	"random32",	"RANDOM32",	"rand32",	"RAND32"	},
			{	"random16",	"RANDOM16",	"rand16",	"RAND16"	},
			{	"random8",	"RANDOM8",	"rand8",	"RAND8"		}
		};
	};



	template<int N, class Protocol, class DBAdapter>
	struct MRANDOM : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd[N-1]);
		};

		const std::string_view *end()   const final{
			return std::end(cmd[N-1]);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 2)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_1);

			auto &container  = blob.container();
			auto &bcontainer = blob.bcontainer();

			auto const varg = 1;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const index = from_string<size_t>(*itk);

				bcontainer.push_back();

				using namespace random_impl_;

				container.push_back(
					to_string(shared::random::get<RTT<N> >(index), bcontainer.back())
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[][6] = {
			{	"mrandom64",	"MRANDOM64",	"mrand64",	"MRAND64"	},
			{	"mrandom32",	"MRANDOM32",	"mrand32",	"MRAND32"	},
			{	"mrandom16",	"MRANDOM16",	"mrand16",	"MRAND16"	},
			{	"mrandom8",	"MRANDOM8",	"mrand8",	"MRAND8"	}
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "counters";

		static void load(RegisterPack &pack){
			using namespace random_impl_;

			return registerCommands<Protocol, DBAdapter, RegisterPack,
				LH<RANDOM >::cmd1	,
				LH<MRANDOM>::cmd1	,
				LH<RANDOM >::cmd2	,
				LH<MRANDOM>::cmd2	,
				LH<RANDOM >::cmd3	,
				LH<MRANDOM>::cmd3	,
				LH<RANDOM >::cmd4	,
				LH<MRANDOM>::cmd4
			>(pack);
		}
	};



} // namespace

