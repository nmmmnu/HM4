#include "base.h"
#include "logger.h"

namespace net::worker::commands::BITSET{
	namespace bit_impl_{
		// for 1 MB * 8 = 8'388'608
		// uint32_t will do fine, but lets be on the safe side
		using size_type = uint64_t;

		constexpr size_type BIT_MAX = hm4::PairConf::MAX_VAL_SIZE * 8 - 1;
	}

	template<class DBAdapter>
	struct BITSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setbit";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setbit",	"SETBIT"	,
			"bitset",	"BITSET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 4)
				return Result::error();

			using namespace bit_impl_;

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			const auto n      = from_string<size_type>(p[2]);

			if (n > BIT_MAX)
				return Result::error();

			size_t  const n_byte		= n / 8;
			size_t  const n_byte_size	= n_byte + 1;
			uint8_t const n_mask		= 1 << (n % 8);
			bool    const bit		= from_string<uint64_t>(p[3]);

			const auto *pair = getPair__(db, key);

			if (pair == nullptr){
				// create new and update

				// do not use c-tor in order to avoid capacity reallocation
				blob.string_val = "";
				blob.string_val.resize(n_byte_size, '\0');

				flipBit__(blob.string_val.data(),
						bit, n_byte, n_mask);

				db.set(key, blob.string_val);

				return Result::ok();

			}else if (pair->getVal().size() >= n_byte_size && db.canUpdateWithHint(pair)){
				// valid pair, update in place

				flipBit__(const_cast<char *>( pair->getVal().data() ),
						bit, n_byte, n_mask);

				db.expHint(pair, 0);

				return Result::ok();
			}else{
				// normal update

				blob.string_val = pair->getVal();

				if (blob.string_val.size() < n_byte_size)
					blob.string_val.resize(n_byte_size, '\0');

				flipBit__(blob.string_val.data(),
						bit, n_byte, n_mask);

				// here size optimization may kick up.
				// see recalc_size__

				db.set(key, blob.string_val);

				return Result::ok();
			}
		}

	private:
		static void flipBit__(char *bits_buffer, bool const bit, size_t const n_byte, uint8_t const n_mask){
			uint8_t *bits = reinterpret_cast<uint8_t *>(bits_buffer);

			if (bit)
				bits[n_byte] |= n_mask;
			else
				bits[n_byte] &= ~n_mask;
		}

		static const hm4::Pair *getPair__(DBAdapter &db, std::string_view const key){
			if (auto it = db.find(key); it && it->isValid(std::true_type{}))
				return & *it;
			else
				return nullptr;
		}

		#if 0
		static auto recalc_size__(std::string_view const s){
			size_t max = 0;

			for(auto it = std::rbegin(s); it != std::rend(s); ++it)
				if (*it == 0)
					++max;
				else
					break;

			return max;
		}
		# endif
	};



	template<class DBAdapter>
	struct BITGET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "getbit";
		constexpr inline static std::string_view cmd[]	= {
			"getbit",	"GETBIT"	,
			"bitget",	"BITGET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 3)
				return Result::error();

			using namespace bit_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return Result::error();

			const auto n     	= from_string<size_type>(p[2]);
			const auto n_byte	= n / 8;

			if (auto it = db.find(key); it && it->isValid(std::true_type{}) && it->getVal().size() >= n_byte){
				const uint8_t *bits = reinterpret_cast<const uint8_t *>(it->getVal().data());

				const auto n_mask  = 1 << (n % 8);

				const bool b = bits[n_byte] & n_mask;

				return Result::ok( b );
			}else{
				return Result::ok( 0 );
			}
		}
	};



	template<class DBAdapter>
	struct BITCOUNT : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "bitcount";
		constexpr inline static std::string_view cmd[]	= {
			"bitcount",	"BITCOUNT"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key); it && it->isValid(std::true_type{}) ){
				auto const &val = it->getVal();

				const uint8_t *bits = reinterpret_cast<const uint8_t *>(val.data());

				// std::accumulate fails to compile

				uint64_t count = 0;
				for(auto it = bits; it != bits + val.size(); ++it)
					count += __builtin_popcount(*it);

				return Result::ok(count);
			}else{
				return Result::ok(0);
			}
		}
	};



	template<class DBAdapter>
	struct BITMAX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "maxbit";
		constexpr inline static std::string_view cmd[]	= {
			"bitmax",	"BITMAX"
		};

		constexpr Result operator()(ParamContainer const &, DBAdapter &, OutputBlob &) const final{
			using namespace bit_impl_;

			return Result::ok(BIT_MAX);
		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				BITSET		,
				BITGET		,
				BITCOUNT	,
				BITMAX
			>(pack);
		}
	};


} // namespace


