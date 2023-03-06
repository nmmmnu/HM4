#include "base.h"
#include "murmur_hash_64a.h"
#include "shared_bitops.h"

#include <algorithm>

namespace net::worker::commands::BF{

	namespace bf_impl_{
		using namespace shared::bit;

		constexpr uint64_t	BIT_MAX		= BitOps::max_bits(hm4::PairConf::MAX_VAL_SIZE);
		constexpr uint64_t	BIT_MIN		= 32;
		constexpr uint8_t	HASH_MAX	= 16;



		void bf_add(uint64_t max_bits, size_t max_hash, char *data, std::string_view val){
			for(size_t seed = 0; seed < max_hash; ++seed){
				auto n = murmur_hash64a(val, seed) % max_bits;

				printf("%zu -> %zu\n", seed, n);

				BitOps{ n }.set(data, 1);
			}
		}

		template<typename It>
		void bf_addMany(uint64_t max_bits, size_t max_hash, char *data, It begin, It end){
			for(auto it = begin; it != end; ++it)
				bf_add(max_bits, max_hash, data, *it);
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

	}



	template<class Protocol, class DBAdapter>
	struct BFADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 5)
				return;

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);
			auto const max_size = BitOps::size(max_bits - 1);

			auto const varg = 4;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &val = *itk;

				if (val.empty())
					return;
			}

			const auto *pair = hm4::getPair_(*db, key, [max_size](bool b, auto it) -> const hm4::Pair *{
				if (b && it->getVal().size() == max_size)
					return & *it;
				else
					return nullptr;
			});



			if (pair == nullptr){
				// create new and update

				char *buffer = blob.buffer_val.data();
				memset(buffer, '\0', max_size);

				bf_addMany(max_bits, max_hash, buffer, std::begin(p) + varg, std::end(p));

				hm4::insert(*db, key, std::string_view{ buffer, max_size });

				return result.set();
			}else if (hm4::canInsertHint<db.TRY_INSERT_HINTS>(*db, pair, max_size)){
				// HINT

				// valid pair, update in place

				char *buffer = const_cast<char *>( pair->getVal().data() );
				bf_addMany(max_bits, max_hash, buffer, std::begin(p) + varg, std::end(p));

				const auto *hint = pair;
				// condition already checked,
				// update the expiration,
				// will always succeed
				hm4::proceedInsertHint(*db, hint, 0, key, pair->getVal());

				return result.set();
			}else{
				// normal update

				char *buffer = blob.buffer_val.data();
				memcpy(buffer, pair->getVal().data(), max_size);

				bf_addMany(max_bits, max_hash, buffer, std::begin(p) + varg, std::end(p));

				hm4::insert(*db, key, std::string_view{ buffer, max_size });

				return result.set();
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfadd",	"BFADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BFRESERVE : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4)
				return;

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
		//	auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);
			auto const max_size = BitOps::size(max_bits - 1);



			bool const ok = hm4::getPair_(*db, key, [max_size](bool b, auto it){
				if (b && it->getVal().size() == max_size)
					return true;
				else
					return false;
			});



			if (!ok){
				// create new and update

				char *buffer = blob.buffer_val.data();
				memset(buffer, '\0', max_size);

				hm4::insert(*db, key, std::string_view{ buffer, max_size });

				return result.set_1();
			}else{
				return result.set_0();
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bfreserve",	"BFRESERVE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BFEXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 5)
				return;

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);

			const auto &val		= p[4];

			if (val.empty())
				return;

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
	struct BFMEXISTS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 5)
				return;

			using namespace bf_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			auto const max_bits = std::clamp<uint64_t	>(from_string<uint64_t	>(p[2]), BIT_MIN,	BIT_MAX		);
			auto const max_hash = std::clamp<size_t		>(from_string<size_t	>(p[3]), 1,		HASH_MAX	);

			auto const varg = 4;
			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &val = *itk;

				if (val.empty())
					return;
			}

			auto data = hm4::getPairVal(*db, key);

			if (data.empty())
				return result.set_0();



			auto &container = blob.container;

			if (container.capacity() < p.size() - varg)
				return;

			container.clear();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &val = *itk;

				bool const b = bf_exists(max_bits, max_hash, data.data(), val);

				container.emplace_back(b ? "1" : "0");
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
		constexpr inline static std::string_view name	= "hll";

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


