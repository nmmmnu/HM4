#include "base.h"
#include "mystring.h"

#include "mortoncurve.h"
#include "hexconvert.h"

#include "shared_stoppredicate.h"
#include "shared_iterations.h"
#include "shared_zset_multi.h"

#include "ilist/txguard.h"

namespace net::worker::commands::MortonCurve8D{

	constexpr uint8_t DIM = 8;

	using ElemType = uint8_t;
	using ZZZType  = uint64_t;

	using MCVector  = std::array<ElemType,		DIM>;
	using SMCVector = std::array<std::string_view,	DIM>;

	static_assert(sizeof(ElemType) * DIM <= sizeof(ZZZType));

	namespace morton_curve_impl_{

		using namespace net::worker::shared::stop_predicate;
		using namespace net::worker::shared::config;

		constexpr size_t MAX_SKIPS = ITERATIONS_IDLE * DIM;

		constexpr size_t scoreSize =  sizeof(ZZZType) * 2;	// ZZZType as hex
		using MCBuffer = std::array<char, scoreSize>;

		using P1 = net::worker::shared::zsetmulti::Permutation1NoIndex;

		constexpr bool isMC8KeyValid(std::string_view keyN, std::string_view keySub){
			return P1::valid(keyN, keySub, scoreSize);
		}

		constexpr std::string_view toHex(ZZZType const z, MCBuffer &buffer){
			using namespace hex_convert;

			constexpr auto opt = options::lowercase | options::nonterminate;

			return hex_convert::toHex<ZZZType, opt>(z, buffer);
		}

		constexpr std::string_view toHex(MCVector const &vvv, MCBuffer &buffer){
			return toHex(
				morton_curve::toMorton8D8(vvv),
				buffer
			);
		}

		std::string_view toHex(SMCVector const &vvvs, MCBuffer &buffer){
			MCVector const vvv{
				from_string<ElemType>(vvvs[ 0]),
				from_string<ElemType>(vvvs[ 1]),
				from_string<ElemType>(vvvs[ 2]),
				from_string<ElemType>(vvvs[ 3]),

				from_string<ElemType>(vvvs[ 4]),
				from_string<ElemType>(vvvs[ 5]),
				from_string<ElemType>(vvvs[ 6]),
				from_string<ElemType>(vvvs[ 7])
			};

			return toHex(vvv, buffer);
		}

		template<size_t N>
		std::string_view formatLine(MCVector const &vvv, uint32_t id, std::array<char, N> &buffer){
			static_assert(N > DIM * (3 + 1) + 8);

			constexpr static std::string_view fmt_mask =
							"{:0>3},{:0>3},{:0>3},{:0>3},"
							"{:0>3},{:0>3},{:0>3},{:0>3},"
							"{:08x}"
			;

			auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask,
											vvv[ 0], vvv[ 1], vvv[ 2], vvv[ 3],
											vvv[ 4], vvv[ 5], vvv[ 6], vvv[ 7],
											id
			);

			if (result.out == std::end(buffer))
				return "PLS_REPORT_A_BUG";
			else
				return { buffer.data(), result.size };
		}



		struct MortonRectangle{
			MCVector v1;
			MCVector v2;

			ZZZType z_min = morton_curve::toMorton8D8(v1);
			ZZZType z_max = morton_curve::toMorton8D8(v2);

			constexpr bool inside(MCVector const &target) const{
				return
					target[ 0] >= v1[ 0] && target[ 0] <= v2[ 0] &&
					target[ 1] >= v1[ 1] && target[ 1] <= v2[ 1] &&
					target[ 2] >= v1[ 2] && target[ 2] <= v2[ 2] &&
					target[ 3] >= v1[ 3] && target[ 3] <= v2[ 3] &&

					target[ 4] >= v1[ 4] && target[ 4] <= v2[ 4] &&
					target[ 5] >= v1[ 5] && target[ 5] <= v2[ 5] &&
					target[ 6] >= v1[ 6] && target[ 6] <= v2[ 6] &&
					target[ 7] >= v1[ 7] && target[ 7] <= v2[ 7]
				;
			}

			auto bigmin(ZZZType zzz) const{
				return morton_curve::computeBigMinFromMorton8D8(zzz, z_min, z_max);
			}

			void print() const{
				logger<Logger::DEBUG>()
						<< v1[ 0] << v2[ 0]
						<< v1[ 1] << v2[ 1]
						<< v1[ 2] << v2[ 2]
						<< v1[ 3] << v2[ 3]

						<< v1[ 4] << v2[ 4]
						<< v1[ 5] << v2[ 5]
						<< v1[ 6] << v2[ 6]
						<< v1[ 7] << v2[ 7]

						<< z_min
						<< z_max
				;
			}
		};



		struct MortonPoint{
			MCVector vector;

			void print() const{
				logger<Logger::DEBUG>()
					<< vector[ 0]
					<< vector[ 1]
					<< vector[ 2]
					<< vector[ 3]

					<< vector[ 4]
					<< vector[ 5]
					<< vector[ 6]
					<< vector[ 7]
				;
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
						toHex(point.vector, buffer)
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
				auto const line = formatLine(point.vector, id++, bcontainer.back());

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

			[[maybe_unused]]
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

				auto const vvv = morton_curve::fromMorton8D8(zzz);

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

				//	logger<Logger::DEBUG>() << "Y >>>" << hex << zzz << vvv[0] << vvv[1] << vvv[2] << vvv[3];
				}else{
					if constexpr(bigmin_optimized){
						if (++skips > MAX_SKIPS){
							if (++retries > MAX_RETRIES)
								return tail(key);

							auto const big_min = rect.bigmin(zzz);

							// done or error
							if (big_min > rect.z_max)
								return tail();

							key_min = createKey(bufferKey[0], big_min);

							// unfortunately the it.operator= is deleted.
							goto start;
						}
					}

				//	logger<Logger::DEBUG>() << "N >>>" << hex << zzz << vvv[0] << vvv[1] << vvv[2] << vvv[3];
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
	struct MC8GET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC8KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				shared::zsetmulti::get<P1>(db, keyN, keySub)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8get",	"MC8GET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8MGET : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8GET key subkey0 subkey1 ...
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

				if (!isMC8KeyValid(keyN, keySub))
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
			"mc8mget",	"MC8MGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8EXISTS : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8GET key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			return shared::zsetmulti::cmdProcessExists(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8exists",	"MC8EXISTS"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8SCORE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8SCORE key subkey

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN   = p[1];
			auto const &keySub = p[2];

			if (keyN.empty() || keySub.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC8KeyValid(keyN, keySub))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			if (auto const hexA = shared::zsetmulti::getIndexes<P1>(db, keyN, keySub); !hexA[0].empty()){

				auto const hex = hexA[0];

				auto const zzz = hex_convert::fromHex<ZZZType>(hex);

				auto const vvv = morton_curve::fromMorton8D8(zzz);

				to_string_buffer_t buffer[DIM];

				SMCVector const container{
					to_string(vvv[ 0], buffer[ 0]),
					to_string(vvv[ 1], buffer[ 1]),
					to_string(vvv[ 2], buffer[ 2]),
					to_string(vvv[ 3], buffer[ 3]),

					to_string(vvv[ 4], buffer[ 4]),
					to_string(vvv[ 5], buffer[ 5]),
					to_string(vvv[ 6], buffer[ 6]),
					to_string(vvv[ 7], buffer[ 7])
				};

				return result.set_container(container);
			}else{
				SMCVector const container{
					"", "", "", "",
					"", "", "", "",
				};

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8score",	"MC8SCORE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8ADD : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8ADD a keySub0 x0 y0 z0 val0 keySub1 x1 y1 z1 val1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			auto const varg  = 2;
			auto const vstep = varg + DIM;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_18);

			const auto &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk +  0);

			//	auto const &v00		= *(itk + 0 + 1);
			//	auto const &v01		= *(itk + 1 + 1);
			//	auto const &v02		= *(itk + 2 + 1);
			//	auto const &v03		= *(itk + 3 + 1);

			//	auto const &v04		= *(itk + 4 + 1);
			//	auto const &v05		= *(itk + 5 + 1);
			//	auto const &v06		= *(itk + 6 + 1);
			//	auto const &v07		= *(itk + 7 + 1);

				auto const &value	= *(itk + 8 + 1);

				if (!isMC8KeyValid(keyN, keySub))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

				if (!hm4::Pair::isValValid(value))
					return result.set_error(ResultErrorMessages::EMPTY_VAL);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &keySub	= *(itk + 0);

				SMCVector const vvvs{
						*(itk +  0 + 1),
						*(itk +  1 + 1),
						*(itk +  2 + 1),
						*(itk +  3 + 1),

						*(itk +  4 + 1),
						*(itk +  5 + 1),
						*(itk +  6 + 1),
						*(itk +  7 + 1)
				};

				auto const &value	= *(itk + DIM + 1);

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
			"mc8add",		"MC8ADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8REM : BaseCommandRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8DEL a subkey0 subkey1 ...

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8rem",		"MC8REM"	,
			"mc8remove",		"MC8REMOVE"	,
			"mc8del",		"MC8DEL"

		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8POINT : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8POINT morton 10 20 30 40 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + DIM + 1 && p.size() != 2 + DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1920);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC8KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const pr = 2;

			MortonPoint const point{
				MCVector{
					from_string<ElemType>(p[ 0 + pr]),
					from_string<ElemType>(p[ 1 + pr]),
					from_string<ElemType>(p[ 2 + pr]),
					from_string<ElemType>(p[ 3 + pr]),

					from_string<ElemType>(p[ 4 + pr]),
					from_string<ElemType>(p[ 5 + pr]),
					from_string<ElemType>(p[ 6 + pr]),
					from_string<ElemType>(p[ 7 + pr])
				}
			};

			auto const sx = DIM + pr;

			auto const count	= myClamp<uint32_t>(p[sx + 0], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 2 + DIM + 1 + 1 ? p[sx + 1] : "";

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
			"mc8point",	"MC8POINT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8RANGENAIVE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC8RANGENAIVE morton 10 10 20 20 30 30 40 40 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + 2 * DIM + 1 && p.size() != 2 + 2 * DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3536);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC8KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const pr = 2;

			MortonRectangle const rect{
				MCVector{
					from_string<ElemType>(p[ 0 + pr]),
					from_string<ElemType>(p[ 2 + pr]),
					from_string<ElemType>(p[ 4 + pr]),
					from_string<ElemType>(p[ 6 + pr]),

					from_string<ElemType>(p[ 8 + pr]),
					from_string<ElemType>(p[10 + pr]),
					from_string<ElemType>(p[12 + pr]),
					from_string<ElemType>(p[14 + pr])
				},
				MCVector{
					from_string<ElemType>(p[ 1 + pr]),
					from_string<ElemType>(p[ 3 + pr]),
					from_string<ElemType>(p[ 5 + pr]),
					from_string<ElemType>(p[ 7 + pr]),

					from_string<ElemType>(p[ 9 + pr]),
					from_string<ElemType>(p[11 + pr]),
					from_string<ElemType>(p[13 + pr]),
					from_string<ElemType>(p[15 + pr])
				}
			};

			auto const sx = 2 * DIM + pr;

			auto const count	= myClamp<uint32_t>(p[sx + 0], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 2 + 2 * DIM + 1 + 1 ? p[sx + 1] : "";

			auto &container  = blob.container();
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
			"mc8rangenaive",	"MC8RANGENAIVE",
			"mc8rangeflat",	"MC8RANGEFLAT"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8RANGE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		// MC2RANGE morton 10 10 20 20 30 30 40 40 10000 [key]

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace morton_curve_impl_;

			if (p.size() != 2 + 2 * DIM + 1 && p.size() != 2 + 2 * DIM + 1 + 1)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3536);

			auto const keyN	= p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (!isMC8KeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			auto const pr = 2;

			MortonRectangle const rect{
				MCVector{
					from_string<ElemType>(p[ 0 + pr]),
					from_string<ElemType>(p[ 2 + pr]),
					from_string<ElemType>(p[ 4 + pr]),
					from_string<ElemType>(p[ 6 + pr]),

					from_string<ElemType>(p[ 8 + pr]),
					from_string<ElemType>(p[10 + pr]),
					from_string<ElemType>(p[12 + pr]),
					from_string<ElemType>(p[14 + pr])
				},
				MCVector{
					from_string<ElemType>(p[ 1 + pr]),
					from_string<ElemType>(p[ 3 + pr]),
					from_string<ElemType>(p[ 5 + pr]),
					from_string<ElemType>(p[ 7 + pr]),

					from_string<ElemType>(p[ 9 + pr]),
					from_string<ElemType>(p[11 + pr]),
					from_string<ElemType>(p[13 + pr]),
					from_string<ElemType>(p[15 + pr])
				}
			};

			auto const sx = 2 * DIM + pr;

			auto const count	= myClamp<uint32_t>(p[sx + 0], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);

			auto const startKey	= p.size() == 2 + 2 * DIM + 1 + 1 ? p[sx + 1] : "";

			auto &container  = blob.container();
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
			"mc8range",	"MC8RANGE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8ENCODE : BaseCommandRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			using namespace morton_curve_impl_;

			if (p.size() != 1 + DIM)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const pr = 1;

			SMCVector const vvvs{
					p[ 0 + pr],
					p[ 1 + pr],
					p[ 2 + pr],
					p[ 3 + pr],

					p[ 4 + pr],
					p[ 5 + pr],
					p[ 6 + pr],
					p[ 7 + pr]
			};

			MCBuffer buffer;

			auto const hex = toHex(vvvs, buffer);

			return result.set(hex);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8encode",	"MC8ENCODE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MC8DECODE : BaseCommandRO<Protocol,DBAdapter>{
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

			auto const vvv = morton_curve::fromMorton8D8(zzz);

			to_string_buffer_t buffer[DIM];

			SMCVector const container{
				to_string(vvv[ 0], buffer[ 0]),
				to_string(vvv[ 1], buffer[ 1]),
				to_string(vvv[ 2], buffer[ 2]),
				to_string(vvv[ 3], buffer[ 3]),

				to_string(vvv[ 4], buffer[ 4]),
				to_string(vvv[ 5], buffer[ 5]),
				to_string(vvv[ 6], buffer[ 6]),
				to_string(vvv[ 7], buffer[ 7])
			};

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mc8decode",	"MC8DECODE"
		};
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "morton_curve_8D";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				MC8GET			,
				MC8MGET			,
				MC8EXISTS		,
				MC8SCORE		,
				MC8ADD			,
				MC8REM			,
				MC8POINT		,
				MC8RANGENAIVE		,
				MC8RANGE		,
				MC8ENCODE		,
				MC8DECODE
			>(pack);
		}
	};



} // namespace


