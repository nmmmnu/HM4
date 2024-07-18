#include "base.h"

#include "geohash.h"
#include "to_fp.h"
#include "logger.h"

#include "fmt/format.h"

#include "shared_zset_multi.h"
#include "shared_iterations.h"

#include "ilist/txguard.h"

#include "stringtokenizer.h"

#include <array>

namespace net::worker::commands::Geo{
	namespace geo_impl_{
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

	} // namespace geo_impl_



	template<class Protocol, class DBAdapter>
	struct GEOADD : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			auto const varg  = 2;
			auto const vstep = 3;

			if (p.size() < varg + vstep || (p.size() - varg) % vstep != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace geo_impl_;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += vstep){
				auto const &name = *(itk + 2);

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

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
		constexpr inline static std::string_view cmd[]	= {
			"geoadd",	"GEOADD"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEOREM : BaseRW<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			using namespace geo_impl_;

			hm4::TXGuard guard{ *db };

			return shared::zsetmulti::cmdProcessRem<P1, GeoIndexController>(p, db, result, blob);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"georem",	"GEOREM"	,
			"georemove",	"GEOREMOVE"	,
			"geodel",	"GEODEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEOGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace geo_impl_;

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
		constexpr inline static std::string_view cmd[]	= {
			"geoget",	"GEOGET"	,
			"geoscore",	"GEOSCORE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEOMGET : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_2);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace geo_impl_;

			auto &container = blob.container();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &name = *itk;

				if (name.empty())
					return result.set_error(ResultErrorMessages::EMPTY_NAME);

				if (!isGeoKeyValid(keyN, name))
					return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
			}

			if (container.capacity() < p.size() - varg)
				return result.set_error(ResultErrorMessages::CONTAINER_CAPACITY);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				auto const &name = *itk;

				container.emplace_back(
					shared::zsetmulti::get<P1, GeoIndexController>(db, keyN, name)
				);
			}

			return result.set_container(container);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"geomget",	"GEOMGET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEORADIUS : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 5)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_4);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace geo_impl_;

			if (!isGeoKeyValid(keyN, "x"))
				return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

			GeoHash::Point me{
					to_geo(p[2]),
					to_geo(p[3])
			};

			auto const radius = to_double_def(p[4]);

			auto &container  = blob.container();
			auto &bcontainer = blob.bcontainer();

			auto const cells = GeoHash::nearbyCells(me, radius, sphere);

		//	logger<Logger::DEBUG>() << "GeoHash searching" << me.lon << me.lat << radius;
		//	logger<Logger::DEBUG>().range(std::begin(cells), std::end(cells));

			uint32_t iterations = 0;
			uint32_t results    = 0;

			for(auto &hash : cells){
				hm4::PairBufferKey bufferKey;

				auto const prefix = P1::template makeKey<0>(bufferKey, DBAdapter::SEPARATOR,
								keyN	,
								"X"	,	// old style not supports txt
								hash
				);

				logger<Logger::DEBUG>() << "GeoHash searching cell" << hash << "prefix" << prefix;

				// send query to DB
				for(auto it = db->find(prefix, std::false_type{}); it != std::end(*db); ++it){
					if (auto const &k = it->getKey(); ! same_prefix(prefix, k))
						break;

					if (++iterations > shared::config::ITERATIONS_LOOPS_MAX)
						return result.set_container(container);

					if (!it->isOK())
						continue;

					auto const t_point = extractPoint(it->getVal());

					auto const dist = distance(me, t_point, sphere);

					if (dist <  radius){
						if (++results > shared::config::ITERATIONS_RESULTS_MAX)
							return result.set_container(container);

						auto const place = extractName( db.SEPARATOR[0], it->getKey() );

						bcontainer.push_back();

						auto const dist_sv = to_string( static_cast<uint64_t>(dist), bcontainer.back());

						container.push_back(place);
						container.push_back(dist_sv);

						logger<Logger::DEBUG>() << "GeoHash result" << place << dist_sv;
					}
				}
			}

			return result.set_container(container);
		}

	private:
		constexpr static auto &sphere =  GeoHash::EARTH_METERS;

		constexpr inline static std::string_view cmd[]	= {
			"georadius",	"GEORADIUS",
			"georadius_ro",	"GEORADIUS_RO",
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEODIST : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			auto const &keyN = p[1];

			if (keyN.empty())
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			using namespace geo_impl_;

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

	private:
		constexpr static auto &sphere =  GeoHash::EARTH_METERS;

		constexpr inline static std::string_view cmd[]	= {
			"geodist",	"GEODIST",
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEOENCODE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			using namespace geo_impl_;

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
		constexpr inline static std::string_view cmd[]	= {
			"geoencode",	"GEOENCODE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GEODECODE : BaseRO<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_1);

			using namespace geo_impl_;

			auto const &hash = p[1];

			auto const[lat, lon] = GeoHash::decode(hash).center();

		//	logger<Logger::DEBUG>() << "GeoHash::decode" << lat << lon << hash;

			buffer_t line_buffer;

			return result.set(
				formatLine(lat, lon, hash, line_buffer)
			);
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
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


