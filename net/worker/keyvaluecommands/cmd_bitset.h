#include "base.h"
#include "logger.h"

namespace net::worker::commands::BITSET{
	namespace bit_impl_{
		// for 1 MB * 8 = 8'388'608
		// uint32_t will do fine, but lets be on the safe side
		using size_type				= uint64_t;

		constexpr size_type BIT_MAX		= hm4::PairConf::MAX_VAL_SIZE * 8 - 1;

		constexpr size_t    BIT_STRING_RESERVE	=  hm4::PairConf::MAX_VAL_SIZE + 16;
	}

	template<class Protocol, class DBAdapter>
	struct BITSET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		BITSET(){
			using namespace bit_impl_;

			string.reserve(BIT_STRING_RESERVE);
		}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return;

			using namespace bit_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			const auto n = from_string<size_type>(p[2]);

			if (n > BIT_MAX)
				return;

			size_t  const n_byte		= n / 8;
			size_t  const n_byte_size	= n_byte + 1;
			uint8_t const n_mask		= 1 << (n % 8);
			bool    const bit		= from_string<uint64_t>(p[3]);

			const auto *pair = hm4::getPairPtr(*db, key);

			if (pair == nullptr){
				// create new and update

				// do not use c-tor in order to avoid capacity reallocation
				string = "";
				string.resize(n_byte_size, '\0');

				flipBit__(string.data(),
						bit, n_byte, n_mask);

				hm4::insert(*db, key, string);

				return result.set();

			}/* else if (pair->getVal().size() >= n_byte_size && db.canUpdateWithHint(pair)){
				TODO HINT

				// valid pair, update in place

				flipBit__(const_cast<char *>( pair->getVal().data() ),
						bit, n_byte, n_mask);

				db.expHint(pair, 0);

				return result.set();
			}*/else{
				// normal update

				string = pair->getVal();

				if (string.size() < n_byte_size)
					string.resize(n_byte_size, '\0');

				flipBit__(string.data(),
						bit, n_byte, n_mask);

				// here size optimization may kick up.
				// see recalc_size__

				hm4::insert(*db, key, string);

				return result.set();
			}
		}

	private:
		std::string string;

	private:
		static void flipBit__(char *bits_buffer, bool const bit, size_t const n_byte, uint8_t const n_mask){
			uint8_t *bits = reinterpret_cast<uint8_t *>(bits_buffer);

			if (bit)
				bits[n_byte] |= n_mask;
			else
				bits[n_byte] &= ~n_mask;
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

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setbit",	"SETBIT"	,
			"bitset",	"BITSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITGET : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			using namespace bit_impl_;

			const auto &key		= p[1];

			if (key.empty())
				return;

			const auto n     	= from_string<size_type>(p[2]);
			const auto n_byte	= n / 8;

			const uint8_t *bits = hm4::getPair_(*db, key, [n_byte](bool b, auto it) -> const uint8_t *{
				if (b && it->getVal().size() >= n_byte)
					return reinterpret_cast<const uint8_t *>(it->getVal().data());
				else
					return nullptr;
			});

			if (bits){
				const auto n_mask  = 1 << (n % 8);

				const bool b = bits[n_byte] & n_mask;

				return result.set(b);
			}else{
				return result.set_0();
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getbit",	"GETBIT"	,
			"bitget",	"BITGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITCOUNT : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto const val = hm4::getPairVal(*db, key); !std::empty(val)){
				const uint8_t *bits = reinterpret_cast<const uint8_t *>(val.data());

				// std::accumulate fails to compile

				uint64_t count = 0;
				for(auto it = bits; it != bits + val.size(); ++it)
					count += __builtin_popcount(*it);

				return result.set(count);
			}else{
				return result.set_0();
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bitcount",	"BITCOUNT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITMAX : Base<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		constexpr void process(ParamContainer const &, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace bit_impl_;

			return result.set(BIT_MAX);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bitmax",	"BITMAX"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "hll";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				BITSET		,
				BITGET		,
				BITCOUNT	,
				BITMAX
			>(pack);
		}
	};



} // namespace


