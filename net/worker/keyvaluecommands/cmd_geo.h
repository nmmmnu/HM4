#include "base.h"

#include "geohash.h"
#include "to_fp.h"
#include "logger.h"

#include "fmt/format.h"

#include "shared_zset_multi.h"
#include "shared_accumulateresults.h"
#include "shared_accumulateresults.h"

#include "ilist/txguard.h"

#include "stringtokenizer.h"

#include <array>

namespace net::worker::commands::Geo{
	namespace impl_{
		namespace {
			auto to_geo(std::string_view s){
				return to_double_def<3, 10>(s);
			}

			// yes, lon 123 is wrong.
			// +179.0123456789,+123.0123456789,sx8dfgbet3sb

			constexpr size_t buffer_t_size = (15 + 1) * 2 + 12;

			constexpr char POINT_SEPARATOR = ',';

			using buffer_t = std::array<char, buffer_t_size>;

			std::string_view formatLine(double lat, double lon, std::string_view hash, buffer_t &buffer){
				constexpr static std::string_view fmt_mask = "{:+.10f},{:+.10f},{}";

				auto const result = fmt::format_to_n(buffer.data(), buffer.size(), fmt_mask, lat, lon, hash);

				if (result.out == std::end(buffer))
					return {};
				else
					return { buffer.data(), result.size };
			}

			auto extractPoint(std::string_view line){
				StringTokenizer const tok{ line, POINT_SEPARATOR };
				auto _ = getForwardTokenizer(tok);

				auto const lat  = to_geo(_());
				auto const lon  = to_geo(_());

				GeoHash::Point r{ lat, lon };

				return r;
			}

			auto extractName(char delimiter, std::string_view line){
				StringTokenizer const tok{ line, delimiter };
				auto _ = getBackwardTokenizer(tok);

				auto const r = _();

				return r;
			}

		} // anonymous namespace

		using P1 = net::worker::shared::zsetmulti::Permutation1NoIndex;

		constexpr bool isGeoKeyValid(std::string_view key, std::string_view name){
			return P1::valid(key, name, GeoHash::MAX_SIZE);
		}

		struct GeoIndexController{
			constexpr static std::string_view encode(std::string_view value){
				return value;
			}

			template<size_t N>
			static std::array<std::string_view, N> decode(std::string_view value){
				static_assert(N == 1);

				// this looks same as extractName,
				// but it do completly different job.

				StringTokenizer const tok{ value, POINT_SEPARATOR };
				auto _ = getBackwardTokenizer(tok);

				auto const r = _();

				return { r };
			}

			constexpr static std::string_view decodeValue(std::string_view value){
				return value;
			}
		};

	} // namespace impl_



	template<class Protocol, class DBAdapter>
	struct GEOADD : BaseCommandRW<Protocol,DBAdapter>{

		GEOADD() : BaseCommandRW<Protocol,DBAdapter>("GEOADD", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			auto const varg  = 2;
			auto const vstep = 3;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &name = *(itk + 2);

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const lat = to_geo(*(itk + 0));
				auto const lon = to_geo(*(itk + 1));

				GeoHash::buffer_t buffer;
				auto const hash = GeoHash::encode(lat, lon, GeoHash::MAX_SIZE, buffer);

				logger<Logger::DEBUG>() << "GeoHash::encode" << lat << lon << hash;

				buffer_t line_buffer;
				auto const line = formatLine(lat, lon, hash, line_buffer);

				auto const &name = *(itk + 2);

				shared::zsetmulti::add<P1, GeoIndexController>(db, keyN, name, { hash }, line);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geoadd",	"GEOADD"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GEOREM : BaseCommandRW<Protocol,DBAdapter>{

		GEOREM() : BaseCommandRW<Protocol,DBAdapter>("GEOREM", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace impl_;

			[[maybe_unused]]
			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1, GeoIndexController>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"georem",	"GEOREM"	,
			"georemove",	"GEOREMOVE"	,
			"geodel",	"GEODEL"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GEOGET : BaseCommandRO<Protocol,DBAdapter>{

		GEOGET() : BaseCommandRO<Protocol,DBAdapter>("GEOGET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto const &name = p[2];

			if (name.empty())
				return result.set_error(ResultErrorMessages::EMPTY_NAME);

			if (!isGeoKeyValid(keyN, name))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			return result.set(
				shared::zsetmulti::get<P1, GeoIndexController>(db, keyN, name)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geoget",	"GEOGET"	,
			"geoscore",	"GEOSCORE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GEOMGET : BaseCommandRO<Protocol,DBAdapter>{

		GEOMGET() : BaseCommandRO<Protocol,DBAdapter>("GEOMGET", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &name = *itk;

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto &container = blob.construct<OutputBlob::Container>();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &name = *itk;

				container.emplace_back(
					shared::zsetmulti::get<P1, GeoIndexController>(db, keyN, name)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geomget",	"GEOMGET"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GEORADIUS : BaseCommandRO<Protocol,DBAdapter>{

		GEORADIUS() : BaseCommandRO<Protocol,DBAdapter>("GEORADIUS", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			if (!isGeoKeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			GeoHash::Point me{
					to_geo(p[2]),
					to_geo(p[3])
			};

			auto const radius = to_double_def(p[4]);

			auto &container  = blob.construct<OutputBlob::Container>();
			auto &bcontainer = blob.construct<OutputBlob::BufferContainer>();

			auto const cells = GeoHash::nearbyCells(me, radius, sphere);

		//	logger<Logger::DEBUG>() << "GeoHash searching" << me.lon << me.lat << radius;
		//	logger<Logger::DEBUG>().range(std::begin(cells), std::end(cells));

			uint32_t results = 0;

			for(auto &hash : cells){
				hm4::PairBufferKey bufferKey;

				auto const prefix = P1::template makeKeyRangeN<0>(bufferKey, DBAdapter::SEPARATOR,
								keyN	,
							//	"X"	,	// old style not supports txt
								hash
				);

				logger<Logger::DEBUG>() << "GeoHash searching cell" << hash << "prefix" << prefix;

				auto pTail = [](std::string_view = ""){};

				auto pPair = [&](auto const &pair) mutable -> bool{
					auto const t_point = extractPoint(pair.getVal());

					auto const dist = distance(me, t_point, sphere);

					if (dist <  radius){
						if (++results > shared::config::ITERATIONS_RESULTS_MAX)
							return false;

						auto const place = extractName( db.SEPARATOR[0], pair.getKey() );

						bcontainer.push_back();

						auto const dist_sv = to_string( static_cast<uint64_t>(dist), bcontainer.back());

						container.push_back(place);
						container.push_back(dist_sv);

						logger<Logger::DEBUG>() << "GeoHash result" << place << dist_sv;
					}

					return true;
				};

				using namespace net::worker::shared::accumulate_results;

				StopPrefixPredicate stop{ prefix };

				// send query to DB

				sharedAccumulatePairs(
					stop			,
					db->find(prefix)	,
					db->end()		,
					pTail			,
					pPair
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr static auto &sphere =  GeoHash::EARTH_METERS;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"georadius",	"GEORADIUS",
			"georadius_ro",	"GEORADIUS_RO",
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEODIST : BaseCommandRO<Protocol,DBAdapter>{

		GEODIST() : BaseCommandRO<Protocol,DBAdapter>("GEODIST", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result){
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace impl_;

			auto const &name1 = p[2];

			{
				auto const &name = name1;

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			auto const &name2 = p[3];

			{
				auto const &name = name2;

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			// ---

			auto const line1 = shared::zsetmulti::get<P1, GeoIndexController>(db, keyN, name1);

			if (line1.empty())
				return result.set(int64_t{-1});

			auto const p1 = extractPoint(line1);

			// ---

			auto const line2 = shared::zsetmulti::get<P1, GeoIndexController>(db, keyN, name2);

			if (line2.empty())
				return result.set(int64_t{-1});

			auto const p2 = extractPoint(line2);

			// ---

			auto const dist = distance(p1, p2, sphere);

			to_string_buffer_t buffer;

			auto const dist_sv = to_string( static_cast<uint64_t>(dist), buffer);

			return result.set(dist_sv);
		}


		constexpr static auto &sphere =  GeoHash::EARTH_METERS;

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geodist",	"GEODIST",
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEOENCODE : BaseCommandRO<Protocol,DBAdapter>{

		GEOENCODE() : BaseCommandRO<Protocol,DBAdapter>("GEOENCODE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, result);
		}

	private:
		static void process__(ParamContainer const &p, Result<Protocol> &result){
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			using namespace impl_;

			auto const lat = to_geo(p[1]);
			auto const lon = to_geo(p[2]);

			GeoHash::buffer_t buffer;
			auto const hash = GeoHash::encode(lat, lon, GeoHash::MAX_SIZE, buffer);

		//	logger<Logger::DEBUG>() << "GeoHash::encode" << lat << lon << hash;

			buffer_t line_buffer;

			return result.set(
				formatLine(lat, lon, hash, line_buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geoencode",	"GEOENCODE"
		};

	};



	template<class Protocol, class DBAdapter>
	struct GEODECODE : BaseCommandRO<Protocol,DBAdapter>{

		GEODECODE() : BaseCommandRO<Protocol,DBAdapter>("GEODECODE", std::begin(cmd__), std::end(cmd__)){}

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			return process__(p, result);
		}

	private:
		static void process__(ParamContainer const &p, Result<Protocol> &result){
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			using namespace impl_;

			auto const &hash = p[1];

			auto const[lat, lon] = GeoHash::decode(hash).center();

		//	logger<Logger::DEBUG>() << "GeoHash::decode" << lat << lon << hash;

			buffer_t line_buffer;

			return result.set(
				formatLine(lat, lon, hash, line_buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"geodecode",	"GEODECODE"
		};

	};






	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "geo";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				GEOADD		,
				GEOREM		,
				GEOGET		,
				GEOMGET		,
				GEORADIUS	,
				GEODIST		,
				GEOENCODE	,
				GEODECODE
			>(pack);
		}
	};



} // namespace


