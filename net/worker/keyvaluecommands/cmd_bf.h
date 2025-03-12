#include "base.h"
#include "murmur_hash_64a.h"
#include "shared_bitops.h"
#include "pair_vfactory.h"

#include <algorithm>

namespace net::worker::commands::BF{
	namespace bf_impl_{

		using namespace shared::bit;

		using Pair = hm4::Pair;

		constexpr uint64_t	BIT_MAX		= BitOps::max_bits(hm4::PairConf::MAX_VAL_SIZE);
		constexpr uint64_t	BIT_MIN		= 32;
		constexpr uint8_t	HASH_MAX	= 16;



		void bf_add(uint64_t max_bits, size_t max_hash, char *data, std::string_view val){
			for(size_t seed = 0; seed < max_hash; ++seed){
				auto n = murmur_hash64a(val, seed) % max_bits;

			//	printf("%zu -> %zu\n", seed, n);

				BitOps{ n }.set(data, 1);
			}
		}

		bool bf_exists(uint64_t max_bits, size_t max_hash, const char *data, std::string_view val){
			for(size_t seed = 0; seed < max_hash; ++seed){
				auto n = murmur_hash64a(val, seed) % max_bits;

			//	printf("%zu -> %zu\n", seed, n);

				if (BitOps{ n }.get(data) == false)
					return false;
			}

			return true;
		}

	} // namespace bf_impl_



	template<class Protocol, class DBAdapter>
	struct BFADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);
			auto const max_size = BitOps::size(max_bits - 1);

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			const auto *pair = hm4::getPairPtrWithSize(*db, key, max_size);

			using MyBFADD_Factory = BFADD_Factory<ParamContainer::iterator>;

			MyBFADD_Factory factory{ key, pair, max_bits, max_hash, std::begin(p) + varg, std::end(p) };

			insertHintVFactory(pair, *db, factory);

			return result.set();
		}

	private:
		template<typename It>
		struct BFADD_Factory : hm4::PairFactory::IFactoryAction<1, 1, BFADD_Factory<It> >{
			using Pair   = hm4::Pair;
			using Base   = hm4::PairFactory::IFactoryAction<1, 1, BFADD_Factory<It> >;
			using BitOps = bf_impl_::BitOps;

			constexpr BFADD_Factory(std::string_view const key, const Pair *pair, uint64_t max_bits, size_t max_hash, It begin, It end) :
							Base::IFactoryAction	(key, BitOps::size(max_bits - 1), pair),
							max_bits			(max_bits	),
							max_hash			(max_hash	),
							begin				(begin		),
							end				(end		){}

			void action(Pair *pair) const{
				using namespace bf_impl_;

				char *data = pair->getValC();

				for(auto it = begin; it != end; ++it)
					bf_add(max_bits, max_hash, data, *it);
			}

		private:
			uint64_t		max_bits;
			size_t			max_hash;
			It			begin;
			It			end;
		};

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfadd",	"BFADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BFRESERVE : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			const auto &key		= p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace bf_impl_;

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
		//	auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);
			auto const max_size = BitOps::size(max_bits - 1);

			hm4::insertV<hm4::PairFactory::Reserve>(*db, key, max_size);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfreserve",	"BFRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BFEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);

			const auto &val		= p[4];

			if (val.empty())
				return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto data = hm4::getPairVal(*db, key);

			if (data.empty())
				return result.set_0();

			return result.set(
				bf_exists(max_bits, max_hash, data.data(), val)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfexists",	"BFEXISTS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BFMEXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 5)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_4);

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);

			auto const varg = 4;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (auto const &val = *itk; val.empty())
					return result.set_error(ResultErrorMessages::EMPTY_VAL);

			auto data = hm4::getPairVal(*db, key);

			auto &container = blob.container();

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			if (data.empty()){
				// no set-size yet :)
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					container.emplace_back("0");
				}
			}else{
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const &val = *itk;

					bool const b = bf_exists(max_bits, max_hash, data.data(), val);

					container.emplace_back(b ? "1" : "0");
				}
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfmexists",	"BFMEXISTS"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "bf";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				BFRESERVE	,
				BFADD		,
				BFEXISTS	,
				BFMEXISTS
			>(pack);
		}
	};



} // namespace


