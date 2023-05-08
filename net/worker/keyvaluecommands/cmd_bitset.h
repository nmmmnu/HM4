#include "base.h"
#include "logger.h"

#include "shared_bitops.h"
#include "smart_memcpy.h"

namespace net::worker::commands::BITSET{
	namespace bit_impl_{
		using namespace shared::bit;

		using Pair = hm4::Pair;

		// for 1 MB * 8 = 8'388'608
		// uint32_t will do fine, but lets be on the safe side
		using size_type				= uint64_t;

		constexpr size_type BIT_MAX		= BitOps::max_bits(hm4::PairConf::MAX_VAL_SIZE);
	}

	template<class Protocol, class DBAdapter>
	struct BITSET : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

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

			using MyBITSET_Factory = BITSET_Factory<ParamContainer::iterator>;

			auto const varg = 2;

			if (pair && hm4::canInsertHint(*db, pair, n_size))
				hm4::proceedInsertHintV<MyBITSET_Factory>(*db, pair, key, n_size, std::begin(p) + varg, std::end(p), pair);
			else
				hm4::insertV<MyBITSET_Factory>(*db, key, n_size, std::begin(p) + varg, std::end(p), pair);

			return result.set();
		}

	private:
		static auto getNSize_(ParamContainer const &p){
			using namespace bit_impl_;

			bool ok = false;
			size_type max = 0;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const n = from_string<size_type>(*itk);

				if (n < BIT_MAX){
					ok  = true;
					max = std::max(max, n);
				}
			}

			return std::make_pair(ok, BitOps::size(max));
		}

	private:
		template<typename It>
		struct BITSET_Factory : hm4::PairFactory::IFactory{
			using Pair = hm4::Pair;

			BITSET_Factory(std::string_view const key, uint64_t val_size, It begin, It end, const Pair *pair) :
							key		(key		),
							val_size	(val_size	),
							begin		(begin		),
							end		(end		),
							old_pair	(pair		){}

			constexpr std::string_view getKey() const final{
				return key;
			}

			constexpr uint32_t getCreated() const final{
				return 0;
			}

			constexpr size_t bytes() const final{
				return Pair::bytes(key.size(), val_size);
			}

			void createHint(Pair *pair) final{
				add_(pair);
			}

			void create(Pair *pair) final{
				Pair::createInRawMemory<1,0,1,1>(pair, key, val_size, 0, 0);
				create_(pair);

				add_(pair);
			}

		private:
			void create_(Pair *pair) const{
				char *data = pair->getValC();

				if (old_pair){
					smart_memcpy(data, val_size, old_pair->getVal());
				}else{
					memset(data, '\0', val_size);
				}
			}

			void add_(Pair *pair) const{
				using namespace bit_impl_;

				char *data = pair->getValC();

				for(auto it = begin; it != end; it += 2){
					auto const n = from_string<size_type>(*it);

					if (n >= BIT_MAX)
						continue;

					bool const bit	= from_string<uint64_t>(*std::next(it));

					BitOps{ n }.set(data, bit);
				}
			}

		private:
			std::string_view	key;
			uint64_t		val_size;
			It			begin;
			It			end;
			const Pair		*old_pair;
		};
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


