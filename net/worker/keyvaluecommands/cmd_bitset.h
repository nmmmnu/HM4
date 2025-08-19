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
	struct BITSET : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// BITSET key 5 1 6 0
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg = 2;
			if (p.size() < 4 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			using namespace bit_impl_;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto const [ok, bytes] = getBytes__(p);

			if (!ok)
				return result.set_error(ResultErrorMessages::INTERNAL_ERROR);

			const auto *pair = hm4::getPairPtr(*db, key);

			using MyBITSET_Factory = BITSET_Factory<ParamContainer::iterator>;

			auto const new_bytes = bytes;

			if (pair && hm4::canInsertHintValSize(*db, pair, new_bytes)){
				auto const val_size = pair->getVal().size();

				hm4::proceedInsertHintV<MyBITSET_Factory>(*db, const_cast<Pair *>(pair), key, val_size, pair, std::begin(p) + varg, std::end(p));
			}else{
				auto const val_size = std::max(
							new_bytes,
							pair ? pair->getVal().size() : 0
				);

				hm4::insertV<MyBITSET_Factory>(*db, key, val_size, pair, std::begin(p) + varg, std::end(p));
			}

			return result.set();
		}

	private:
		static auto getBytes__(ParamContainer const &p){
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
		struct BITSET_Factory : hm4::PairFactory::IFactoryAction<1, 0, BITSET_Factory<It> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<1, 0, BITSET_Factory<It> >;

			constexpr BITSET_Factory(std::string_view const key, uint64_t val_size, const Pair *pair, It begin, It end) :
							Base::IFactoryAction	(key, val_size, pair),
							begin				(begin		),
							end				(end		){}

			void action(Pair *pair) const{
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
			It			begin;
			It			end;
		};
			private:
		constexpr inline static std::string_view cmd[]	= {
			"setbit",	"SETBIT"	,
			"bitset",	"BITSET"	,
			"bitmset",	"BITMSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct BITGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			using namespace bit_impl_;

			const auto &key		= p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			const auto n     	= from_string<size_type>(p[2]);

			BitOps const bitops{ n };

			const char *data = hm4::getPairOK_(*db, key, [size = bitops.size()](bool b, const auto *p) -> const char *{
				if (b && p->getVal().size() >= size)
					return p->getVal().data();
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
	struct BITMGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			using namespace bit_impl_;

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			auto &container = blob.construct<OutputBlob::Container>();

			auto const varg = 2;

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			const char *data = hm4::getPairOK_(*db, key, [](bool b, const auto *p) -> const char *{
				if (b)
					return p->getVal().data();
				else
					return nullptr;
			});

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
	struct BITCOUNT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			const auto &key = p[1];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);



			if (auto const val = hm4::getPairVal(*db, key); !std::empty(val)){
				const uint8_t *bits = reinterpret_cast<const uint8_t *>(val.data());

				// std::accumulate fails to compile

				uint64_t count = 0;
				for(auto it = bits; it != bits + val.size(); ++it)
					count += (uint64_t) __builtin_popcount(*it);

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
	struct BITMAX : BaseCommandRO<Protocol,DBAdapter>{
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
		constexpr inline static std::string_view name	= "bitset";

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


