#include "base.h"
#include "mystring.h"

#include "mortoncurve.h"
#include "hexconvert.h"

#include "shared_stoppredicate.h"
#include "shared_iterations.h"
#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::MortonCurve2D{

	constexpr uint8_t DIM = 2;

	using ZZZType = uint64_t;

	using MCVector  = std::array<uint32_t,		DIM>;
	using SMCVector = std::array<std::string_view,	DIM>;

	namespace morton_curve_impl_{

		using namespace net::worker::shared::stop_predicate;
		using namespace net::worker::shared::config;

		constexpr size_t scoreSize =  sizeof(ZZZType) * 2;	// ZZZType as hex
		using MCBuffer = std::array<char, scoreSize>;

		using P1 = net::worker::shared::zsetmulti::Permutation1NoIndex;

		constexpr bool isMC2KeyValid(std::string_view keyN, std::string_view keySub){
			return P1::valid(keyN, keySub, scoreSize);
		}

		constexpr std::string_view toHex(ZZZType const zzz, MCBuffer &buffer){
			using namespace hex_convert;

			constexpr auto opt = options::lowercase | options::nonterminate;

			return hex_convert::toHex<ZZZType, opt>(zzz, buffer);
		}

		constexpr std::string_view toHex(MCVector const &vvv, MCBuffer &buffer){
			return toHex(
				morton_curve::toMorton2D(vvv[0], vvv[1]),
				buffer
			);
		}

		std::string_view toHex(SMCVector const &vvvs, MCBuffer &buffer){
			MCVector const vvv{
				from_string<uint32_t>(vvvs[0]),
				from_string<uint32_t>(vvvs[1])
			};

			return toHex(vvv, buffer);
		}

		template<size_t N>
		std::string_view formatLine(MCVector const &vvv, uint32_t id, std::array<char, N> &buffer){
			static_assert(N > DIM * (10 + 1) + 8);

			constexpr static std::string_view fmt_mask = "{:0>10},{:0>10},{:08x}";

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, vvv[0], vvv[1], id);

			if (result.out == std::end(buffer))
				return "PLS_REPORT_A_BUG";
			else
				return { buffer.data(), result.size };
		}



		struct MortonRectangle{
			uint32_t x1;
			uint32_t x2;

			uint32_t y1;
			uint32_t y2;

			ZZZType z_min = morton_curve::toMorton2D(x1, y1);
			ZZZType z_max = morton_curve::toMorton2D(x2, y2);

			constexpr MortonRectangle(uint32_t x1, uint32_t x2, uint32_t y1, uint32_t y2) :
					x1(x1), x2(x2),
					y1(y1), y2(y2){}

			constexpr bool inside(MCVector const &vvv) const{
				return inside_(vvv[0], vvv[1]);
			}

			auto bigmin(ZZZType zzz) const{
				return morton_curve::computeBigMinFromMorton2D(zzz, z_min, z_max);
			}

			void print() const{
				logger<Logger::DEBUG>()
						<< x1 << x2
						<< y1 << y2
						<< z_min
						<< z_max
				;
			}
		private:
			constexpr bool inside_(uint32_t x, uint32_t y) const{
				return
					x >= x1 && x <= x2 &&
					y >= y1 && y <= y2
				;
			}
		};



		struct MortonPoint{
			uint32_t x;
			uint32_t y;

		//	constexpr bool inside(uint32_t x, uint32_t y) const{
		//		return
		//			this->x == x &&
		//			this->y == y
		//		;
		//	}

			constexpr auto vector() const{
				return MCVector{ x, y };
			}

			void print() const{
				logger<Logger::DEBUG>() << x << y;
			}
		};



		template<class DBAdapter>
		void mortonSearchPoint(
				DBAdapter &db,
				OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer,
				std::string_view keyN, uint32_t count,
				MortonPoint const &point, std::string_view startKey){

			hm4::PairBufferKey bufferKeyPrefix;

			auto const prefix = [&](){
				if (!startKey.empty())
					return startKey;

				MCBuffer buffer;

				return P1::makeKey(bufferKeyPrefix, DBAdapter::SEPARATOR,
						keyN			,
						"X"			,	// old style not supports txt
						toHex(point.vector(),buffer)
				);
			}();

			StopPrefixPredicate stop(prefix);

			uint32_t iterations	= 0;
			uint32_t results	= 0;
			uint32_t id		= 0;

			auto tail = [&](std::string_view const pkey = ""){
				container.emplace_back(pkey);
			};

			for(auto it = db->find(prefix); it != db->end(); ++it){
				auto const &key = it->getKey();

				if (stop(key))
					return tail(); // no tail

				if (++iterations > ITERATIONS_LOOPS_MAX)
					return tail(key);

				if (! it->isOK())
					continue;

				// because of the prefix check in StopPrefixPredicate,
				// the point is always correct.

				if (++results > count)
					return tail(key);

				auto const &val = it->getVal();

				bcontainer.push_back();
				auto const line = formatLine(point.vector(), id++, bcontainer.back());

				container.emplace_back(line);
				container.emplace_back(val);
			}

			return tail();
		}



		#ifdef __GNUG__
			// this silence the warning for unused label.
			// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113582
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wunused-label"
		#endif

		template<bool bigmin_optimized, class DBAdapter>
		void mortonSearch(
				DBAdapter &db, OutputBlob::Container &container, OutputBlob::BufferContainer &bcontainer,
				std::string_view keyN, uint32_t count,
				MortonRectangle const &rect, std::string_view startKey){

			constexpr uint32_t MAX_RETRIES = 9;

			auto createKey = [keyN](hm4::PairBufferKey &bufferKey, ZZZType zzz){
				MCBuffer buffer;

				return P1::makeKey(bufferKey, DBAdapter::SEPARATOR,
						keyN			,
						"X"			,	// old style not supports txt
						toHex(zzz, buffer)
				);
			};

			hm4::PairBufferKey bufferKey[2];

			auto       key_min = 	! startKey.empty() ? startKey :
						createKey(bufferKey[0], rect.z_min );
			auto const key_max =	createKey(bufferKey[1], rect.z_max );

			StopRangePrefixPredicate stop(key_max);

		//	rect.print();

			uint32_t iterations	= 0;
			uint32_t results	= 0;
			uint32_t skips		= 0;
			uint32_t retries	= 0;
			uint32_t id		= 0;

			auto tail = [&](std::string_view const pkey = ""){
				logger<Logger::DEBUG>() << "MORTON SEARCH DONE >>>"
					<< "iterations"	<< iterations
					<< "results"	<< results
					<< "retries"	<< retries
				;

				container.emplace_back(pkey);
			};


		start: // label for goto :)

			for(auto it = db->find(key_min); it != db->end(); ++it){
				auto const &key = it->getKey();

				if (stop(key))
					return tail(); // no tail

				if (++iterations > ITERATIONS_LOOPS_MAX)
					return tail(key);

				if (! it->isOK())
					continue;

				auto const hexA = P1::decodeIndex(DBAdapter::SEPARATOR,
							after_prefix(P1::sizeKey(keyN), key));

				auto const hex  = hexA[0];

				auto const zzz = hex_convert::fromHex<ZZZType>(hex);

				auto const vvv = morton_curve::fromMorton2D(zzz);

				if (rect.inside(vvv)){
					if (++results > count)
						return tail(key);

					auto const &val = it->getVal();

					bcontainer.push_back();
					auto const line = formatLine(vvv, id++, bcontainer.back());

					container.emplace_back(line);
					container.emplace_back(val);

					if constexpr(bigmin_optimized){
						skips = 0;
					}

				//	logger<Logger::DEBUG>() << "Y >>>" << hex << zzz << vvv[0] << vvv[1];
				}else{
					if constexpr(bigmin_optimized){
						if (++skips > ITERATIONS_IDLE){
							if (++retries > MAX_RETRIES)
								return tail(key);

							auto const big_min = rect.bigmin(zzz);

							logger<Logger::DEBUG>() << "BIGMIN" << zzz << rect.z_min << rect.z_max << big_min;

							// done or error
							if (big_min > rect.z_max)
								return tail();

							key_min = createKey(bufferKey[0], big_min);

							// unfortunately the it.operator= is deleted.
							goto start;
						}
					}

				//	logger<Logger::DEBUG>() << "N >>>" << hex << zzz << vvv[0] << vvv[1];
				}
			}

			return tail();
		}

		#ifdef __GNUG__
			// this silence the warning for unused label.
			// see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=113582
			#pragma GCC diagnostic pop
		#endif


	} // namespace morton_curve_impl_



	template<class Protocol, class DBAdapter>
	struct MC2GET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				shared::zsetmulti::get<P1>(db, keyN, keySub)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2get",	"MC2GET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2MGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2GET key subkey0 subkey1 ...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

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

				if (!isMC2KeyValid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}



			auto &container = blob.container();



			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &keySub = *itk;

				container.emplace_back(
					shared::zsetmulti::get<P1>(db, keyN, keySub)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2mget",	"MC2MGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2EXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			return shared::zsetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2exists",	"MC2EXISTS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2SCORE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2SCORE key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (auto const hexA = shared::zsetmulti::getIndexes<P1>(db, keyN, keySub); !hexA[0].empty()){

				auto const hex = hexA[0];

				auto const zzz   = hex_convert::fromHex<ZZZType>(hex);

				auto const vvv = morton_curve::fromMorton2D(zzz);

				to_string_buffer_t buffer[DIM];

				SMCVector container{
					to_string(vvv[0], buffer[0]),
					to_string(vvv[1], buffer[1])
				};

				return result.set_container(container);
			}else{
				SMCVector const container{
					"",
					""
				};

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2score",	"MC2SCORE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2ADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2ADD a keySub0 x0 y0 val0 keySub1 x1 y1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			auto const varg  = 2;
			auto const vstep = 4;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_5);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk + 0);
			//	auto const &x		= *(itk + 1);
			//	auto const &y		= *(itk + 2);
				auto const &value	= *(itk + 3);

				if (!isMC2KeyValid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk + 0);

				SMCVector const vvvs{
						*(itk + 1),
						*(itk + 2)
				};

				auto const &value	= *(itk + 3);

				MCBuffer buffer;

				auto const score	= toHex(vvvs, buffer);

				shared::zsetmulti::add<P1>(
						db,
						keyN, keySub, { score }, value
				);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2add",		"MC2ADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2REM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2DEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2rem",		"MC2REM"	,
			"mc2remove",		"MC2REMOVE"	,
			"mc2del",		"MC2DEL"

		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2POINT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2POINT morton 10 20 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + DIM + 1 && p.size() != 2 + DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_45);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			MortonPoint const point{
				from_string<uint32_t>(p[2]),
				from_string<uint32_t>(p[3])
			};

			auto const count	= myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 6 ? p[5] : "";

			auto &container  = blob.container();
			auto &bcontainer = blob.bcontainer();

			mortonSearchPoint(
				db,
				container, bcontainer,
				keyN, count,
				point,
				startKey
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2point",	"MC2POINT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2RANGENAIVE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2RANGENAIVE morton 10 10 20 20 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + 2 * DIM + 1 && p.size() != 2 + 2 * DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_67);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			MortonRectangle const rect{
				from_string<uint32_t>(p[2]),
				from_string<uint32_t>(p[3]),

				from_string<uint32_t>(p[4]),
				from_string<uint32_t>(p[5])
			};

			auto const count	= myClamp<uint32_t>(p[6], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 8 ? p[7] : "";

			auto &container = blob.container();
			auto &bcontainer = blob.bcontainer();

			mortonSearch<false>(
				db,
				container, bcontainer,
				keyN, count,
				rect, startKey
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2rangenaive",	"MC2RANGENAIVE" ,
			"mc2rangeflat",		"MC2RANGEFLAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2RANGE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2RANGE morton 10 10 20 20 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + 2 * DIM + 1 && p.size() != 2 + 2 * DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_67);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC2KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			MortonRectangle const rect{
				from_string<uint32_t>(p[2]),
				from_string<uint32_t>(p[3]),

				from_string<uint32_t>(p[4]),
				from_string<uint32_t>(p[5])
			};

			auto const count	= myClamp<uint32_t>(p[6], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 8 ? p[7] : "";

			auto &container = blob.container();
			auto &bcontainer = blob.bcontainer();

			mortonSearch<true>(
				db, container, bcontainer,
				keyN, count,
				rect, startKey
			);

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2range",	"MC2RANGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2ENCODE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 1 + DIM)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			SMCVector const vvvs{ p[1], p[2] };

			MCBuffer buffer;

			auto const hex = toHex(vvvs, buffer);

			return result.set(hex);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2encode",	"MC2ENCODE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC2DECODE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			auto const &hex = p[1];

			auto const zzz = hex_convert::fromHex<ZZZType>(hex);

			auto const vvv = morton_curve::fromMorton2D(zzz);

			to_string_buffer_t buffer[DIM];

			SMCVector const container{
				to_string(vvv[0], buffer[0]),
				to_string(vvv[1], buffer[1])
			};

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc2decode",	"MC2DECODE"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "morton_curve_2d";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MC2GET			,
				MC2MGET			,
				MC2EXISTS		,
				MC2SCORE		,
				MC2ADD			,
				MC2REM			,
				MC2POINT		,
				MC2RANGENAIVE		,
				MC2RANGE		,
				MC2ENCODE		,
				MC2DECODE
			>(pack);
		}
	};



} // namespace

