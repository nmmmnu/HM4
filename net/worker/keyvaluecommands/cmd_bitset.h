#include "base.h"
#include "logger.h"

namespace net::worker::shared::bit{

	struct BitOps{
		size_t  const n_byte;
		uint8_t const n_mask;

		constexpr BitOps(size_t n) :
				n_byte(n / 8		),
				n_mask(1 << (n % 8)	){}

		constexpr size_t size() const{
			return n_byte + 1;
		}

		void set(char *buffer, bool bit) const{
			uint8_t *bits = reinterpret_cast<uint8_t *>(buffer);

			if (bit)
				bits[n_byte] |= n_mask;
			else
				bits[n_byte] &= ~n_mask;
		}

		bool get(const char *buffer) const{
			const uint8_t *bits = reinterpret_cast<const uint8_t *>(buffer);

			return bits[n_byte] & n_mask;
		}

		constexpr static size_t max(size_t buffer_size){
			return buffer_size * 8 - 1;
		}
	};

}

namespace net::worker::commands::BITSET{
	namespace bit_impl_{
		using namespace shared::bit;

		// for 1 MB * 8 = 8'388'608
		// uint32_t will do fine, but lets be on the safe side
		using size_type				= uint64_t;

		constexpr size_type BIT_MAX		= BitOps::max(hm4::PairConf::MAX_VAL_SIZE);

		constexpr size_t    BIT_STRING_RESERVE	= hm4::PairConf::MAX_VAL_SIZE + 16;
	}

	template<class Protocol, class DBAdapter>
	struct BITSET : BaseRW<Protocol,DBAdapter>{
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
			// should be even number arguments
			// bitset key 5 1 6 0
			if (p.size() < 4 || p.size() % 2 == 1)
				return;

			using namespace bit_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto const [ok, n_size] = getNSize_(p);

			if (!ok)
				return;

			const auto *pair = hm4::getPairPtr(*db, key);

			if (pair == nullptr){
				// create new and update

				// do not use c-tor in order to avoid capacity reallocation
				string = "";
				string.resize(n_size, '\0');

				char *data = string.data();
				updateAll_(p, data);

				hm4::insert(*db, key, string);

				return result.set();

			}else if (hm4::canInsertHint<db.TRY_INSERT_HINTS>(*db, pair, n_size)){
				// HINT

				// valid pair, update in place

				char *data = const_cast<char *>( pair->getVal().data() );
				updateAll_(p, data);

				const auto *hint = pair;
				// condition already checked,
				// update the expiration,
				// will always succeed
				hm4::proceedInsertHint(*db, hint, 0, key, pair->getVal());

				return result.set();
			}else{
				// normal update

				string = pair->getVal();

				if (string.size() < n_size)
					string.resize(n_size, '\0');

				char *data = string.data();
				updateAll_(p, data);

				hm4::insert(*db, key, string);

				return result.set();
			}
		}

	private:
		static auto getNSize_(ParamContainer const &p){
			using namespace bit_impl_;

			bool ok = false;
			size_type max = 0;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n <= BIT_MAX){
					ok  = true;
					max = std::max(max, n);
				}
			}

			BitOps const bitops{ max };

			return std::make_pair(ok, bitops.size());
		}

		static void updateAll_(ParamContainer const &p, char *data){
			using namespace bit_impl_;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n > BIT_MAX)
					continue;

				bool const bit	= from_string<uint64_t>(*std::next(itk));

				BitOps const bitops{ n };

				bitops.set(data, bit);
			}
		}

	private:
		std::string string;

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setbit",	"SETBIT"	,
			"bitset",	"BITSET"	,
			"bitmset",	"BITMSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITGET : BaseRO<Protocol,DBAdapter>{
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

			BitOps const bitops{ n };

			const char *data = hm4::getPair_(*db, key, [size = bitops.size()](bool b, auto it) -> const char *{
				if (b && it->getVal().size() >= size)
					return it->getVal().data();
				else
					return nullptr;
			});

			if (data){
				const bool b = bitops.get(data);

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
	struct BITMGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return;

			using namespace bit_impl_;

			const auto &key = p[1];

			if (key.empty())
				return;

			auto &container = blob.container;

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return;

			const char *data = hm4::getPair_(*db, key, [](bool b, auto it) -> const char *{
				if (b)
					return it->getVal().data();
				else
					return nullptr;
			});

			container.clear();

			if (data){
				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					const auto n = from_string<size_type>(*itk);

					BitOps const bitops{ n };

					const bool b = bitops.get(data);

					container.emplace_back(b ? "1" : "0");
				}

				return result.set_container(container);
			}else{
				// return zeroes
				container.assign(p.size() - varg, "0");

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"bitmget",	"BITMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITCOUNT : BaseRO<Protocol,DBAdapter>{
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
	struct BITMAX : BaseRO<Protocol,DBAdapter>{
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
				BITMGET		,
				BITCOUNT	,
				BITMAX
			>(pack);
		}
	};



} // namespace


