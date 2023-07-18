#include "geohash.h"

#include <cassert>
#include <stdexcept>

namespace GeoHash {
	// based on https://www.movable-type.co.uk/scripts/geohash.html

	constexpr double LAT_MIN		=  -90;
	constexpr double LAT_MAX		=   90;
	constexpr double LON_MIN		= -180;
	constexpr double LON_MAX		=  180;

	constexpr inline static std::string_view BASE32 = "0123456789bcdefghjkmnpqrstuvwxyz"; // geohash-specific Base32 map

	// ------------------------------

	std::string_view encode(double lat, double lon, size_t precision, buffer_t &buffer) noexcept{
		assert(precision > 0 && precision <= MAX_SIZE);

		size_t  ixb = 0;	// index of the buffer
		size_t  idx = 0;	// index into base32 map
		uint8_t bit = 0;	// each char holds 5 bits

		auto evenBit = true;

		auto latMin = LAT_MIN;
		auto latMax = LAT_MAX;
		auto lonMin = LON_MIN;
		auto lonMax = LON_MAX;

		while (ixb < precision){
			if (evenBit){
				// bisect E-W longitude
				auto const lonMid = (lonMin + lonMax) / 2;

				if (lon >= lonMid){
					idx	 = idx * 2 + 1;
					lonMin = lonMid;
				} else {
					idx	 = idx * 2;
					lonMax = lonMid;
				}
			}else{
				// bisect N-S latitude
				auto const latMid = (latMin + latMax) / 2;

				if (lat >= latMid){
					idx	 = idx * 2 + 1;
					latMin = latMid;
				} else {
					idx	 = idx * 2;
					latMax = latMid;
				}
			}

			evenBit = !evenBit;

			if (++bit == 5){
				// 5 bits gives us a character, append it and start over
				buffer[ixb++] = BASE32[idx];
				bit = 0;
				idx = 0;
			}
		}

		buffer[ixb] = '\0';

		return std::string_view{ buffer.data(), ixb };
	}

	Rectangle decode(std::string_view hash){
		assert(hash.size() > 0 && hash.size() <= MAX_SIZE);

		auto evenBit = true;

		auto latMin = LAT_MIN;
		auto latMax = LAT_MAX;
		auto lonMin = LON_MIN;
		auto lonMax = LON_MAX;

		for (size_t i = 0; i < hash.size(); ++i){
			auto const chr = hash[i];
			auto const idx = BASE32.find(chr, 0);

			if (idx == BASE32.npos)
				throw std::logic_error("Invalid geohash");

			for (ssize_t n = 4; n >= 0; --n){
				auto const bitN = (idx >> n) & 1;

				if (evenBit){
					// longitude
					auto const lonMid = (lonMin + lonMax) / 2;

					if (bitN){
						lonMin = lonMid;
					}else{
						lonMax = lonMid;
					}
				}else{
					// latitude
					auto const latMid = (latMin + latMax) / 2;

					if (bitN){
						latMin = latMid;
					}else{
						latMax = latMid;
					}
				}

				evenBit = !evenBit;
			}
		}

		return {
			Point{ latMin, lonMin },
			Point{ latMax, lonMax }
		};
	}

	namespace{

		std::string_view adjacent_calc_(size_t const size, Direction direction_, buffer_t &buffer){
			// based on github.com/davetroy/geohash-js

			constexpr static std::string_view neighbour[4][2] {
				{ "p0r21436x8zb9dcf5h7kjnmqesgutwvy", "bc01fg45238967deuvhjyznpkmstqrwx" },
				{ "14365h7k9dcfesgujnmqp0r2twvyx8zb", "238967debc01fg45kmstqrwxuvhjyznp" },
				{ "bc01fg45238967deuvhjyznpkmstqrwx", "p0r21436x8zb9dcf5h7kjnmqesgutwvy" },
				{ "238967debc01fg45kmstqrwxuvhjyznp", "14365h7k9dcfesgujnmqp0r2twvyx8zb" }
			};

			constexpr static std::string_view border[4][2] {
				{ "prxz",	"bcfguvyz"	},
				{ "028b",	"0145hjnp"	},
				{ "bcfguvyz",	"prxz"		},
				{ "0145hjnp",	"028b"		}
			};

			auto const direction = static_cast<uint8_t>(direction_);
			auto const lastCh	 = buffer[size - 1];
			auto const type		= size % 2;

			// check for edge-cases which don't share common prefix
			if (border[direction][type].find(lastCh) != std::string_view::npos && size - 1 > 0)
				adjacent_calc_(size - 1, direction_, buffer);

			// append letter for direction to parent
			buffer[size - 1] = BASE32[ neighbour[direction][type].find(lastCh) ];

			return { buffer.data(), size };
		}

	} // anonymous namespace

	std::string_view adjacent(std::string_view geohash, Direction direction, buffer_t &buffer) noexcept{
		assert(geohash.size() > 0 && geohash.size() <= MAX_SIZE);

		// copy into buffer
		std::copy(std::begin(geohash), std::end(geohash), std::begin(buffer));

		return adjacent_calc_(geohash.size(), direction, buffer);
	}

	double distance_radians(double lat1, double lon1, double lat2, double lon2) noexcept{
		// Haversine Formula
		double const delta_lon = lon2 - lon1;
		double const delta_lat = lat2 - lat1;

		double const result =
			pow(sin(delta_lat / 2), 2) +
			pow(sin(delta_lon / 2), 2) * cos(lat1) * cos(lat2)
		;

		return 2 * asin(sqrt(result));
	}

	// ------------------------------

	namespace{

		size_t selectCellsSize_(double radius, GeoSphere const &sphere){
			if (radius > sphere.cells[0])
				return 0;

			size_t i = 0;
			for(auto &cell : sphere.cells){
				// works OK until lat = 60.0,
				// but we shrink the cells sizes by 66% so is OK until 70.0
				if (radius > cell)
					break;
				++i;
			}

			return i;
		}
	}

	HashVector nearbyCells(double lat, double lon, double radius, GeoSphere const &sphere){
		constexpr bool ENABLE_OPTIMIZATIONS = 1;

		HashVector v;

		auto const size = selectCellsSize_(radius, sphere);

		if (!size){
			v.size = 0;

			return v;
		}

		v.data[0] = encode(lat, lon, size, v.buffer[0]);

		// we can do this, but code becomes unreadable
		//
		// auto adj = [&v](size_t index, size_t src, Direction dir) mutable{
		//	v.data[index] = adjacent(v.data[src], dir, v.data[buffer]);
		// };

		if constexpr(ENABLE_OPTIMIZATIONS){
			auto const bbox = decode(v.data[0]);

			auto const bbox_w = bbox.width(sphere);
			auto const bbox_h = bbox.height(sphere);

			if (bbox_h > 2 * radius){
				auto const bbox_center = bbox.center();

				if (bbox_w > 2 * radius){
					// OPTIMIZED - 2 x 2 = 4

					v.size = 4;

					if (lat < bbox_center.lat){
						// north
						if (lon < bbox_center.lon){
							// north west

							v.data[1] = adjacent(v.data[0],	Direction::n, v.buffer[1]);
							v.data[2] = adjacent(v.data[0],	Direction::w, v.buffer[2]);
							v.data[3] = adjacent(v.data[1],	Direction::w, v.buffer[3]);

							return v;
						}else{
							// north east

							v.data[1] = adjacent(v.data[0],	Direction::n, v.buffer[1]);
							v.data[2] = adjacent(v.data[0],	Direction::e, v.buffer[2]);
							v.data[3] = adjacent(v.data[1],	Direction::e, v.buffer[3]);

							return v;
						}
					}else{
						// south
						if (lon < bbox_center.lon){
							// south west

							v.data[1] = adjacent(v.data[0],	Direction::s, v.buffer[1]);
							v.data[2] = adjacent(v.data[0],	Direction::w, v.buffer[2]);
							v.data[3] = adjacent(v.data[1],	Direction::w, v.buffer[3]);

							return v;
						}else{
							// south east

							v.data[1] = adjacent(v.data[0],	Direction::s, v.buffer[1]);
							v.data[2] = adjacent(v.data[0],	Direction::e, v.buffer[2]);
							v.data[3] = adjacent(v.data[1],	Direction::e, v.buffer[3]);

							return v;
						}
					}
				}

				// This still apply:
				//
				// bbox_h > 2 * radius

				// OPTIMIZED - 3 x 2 = 6

				v.size = 6;

				if (lat > bbox_center.lat){
					// north

					v.data[1] = adjacent(v.data[0],	Direction::n, v.buffer[1]);

					v.data[2] = adjacent(v.data[0],	Direction::e, v.buffer[2]);
					v.data[3] = adjacent(v.data[0],	Direction::w, v.buffer[3]);

					v.data[4] = adjacent(v.data[1],	Direction::e, v.buffer[4]);
					v.data[5] = adjacent(v.data[1],	Direction::w, v.buffer[5]);

					return v;
				}else{
					// south

					v.data[1] = adjacent(v.data[0],	Direction::s, v.buffer[1]);

					v.data[2] = adjacent(v.data[0],	Direction::e, v.buffer[2]);
					v.data[3] = adjacent(v.data[0],	Direction::w, v.buffer[3]);

					v.data[4] = adjacent(v.data[1],	Direction::e, v.buffer[4]);
					v.data[5] = adjacent(v.data[1],	Direction::w, v.buffer[5]);

					return v;
				}
			}

			if (bbox_w > 2 * radius){
				auto const bbox_center = bbox.center();

				// OPTIMIZED - 2 x 3 = 6

				// no need to check lat, it was already checked

				v.size = 6;

				if (lon < bbox_center.lon){
					// west

					v.data[1] = adjacent(v.data[0],	Direction::w, v.buffer[1]);

					v.data[2] = adjacent(v.data[0],	Direction::n, v.buffer[2]);
					v.data[3] = adjacent(v.data[0],	Direction::s, v.buffer[3]);

					v.data[4] = adjacent(v.data[1],	Direction::n, v.buffer[4]);
					v.data[5] = adjacent(v.data[1],	Direction::s, v.buffer[5]);

					return v;
				}else{
					// east

					v.data[1] = adjacent(v.data[0],	Direction::e, v.buffer[1]);

					v.data[2] = adjacent(v.data[0],	Direction::n, v.buffer[2]);
					v.data[3] = adjacent(v.data[0],	Direction::s, v.buffer[3]);

					v.data[4] = adjacent(v.data[1],	Direction::n, v.buffer[4]);
					v.data[5] = adjacent(v.data[1],	Direction::s, v.buffer[5]);

					return v;
				}

			}
		} // if constexpr(ENABLE_OPTIMIZATIONS)

		// NOT OPTIMIZED 3 x 3 = 9

		v.size = 9;

		v.data[1] = adjacent(v.data[0],	Direction::n, v.buffer[1]); // n
		v.data[2] = adjacent(v.data[0],	Direction::s, v.buffer[2]); // s
		v.data[3] = adjacent(v.data[0],	Direction::e, v.buffer[3]); // e
		v.data[4] = adjacent(v.data[0],	Direction::w, v.buffer[4]); // w

		v.data[5] = adjacent(v.data[1],	Direction::e, v.buffer[5]); // ne
		v.data[6] = adjacent(v.data[1],	Direction::w, v.buffer[6]); // nw

		v.data[7] = adjacent(v.data[2],	Direction::e, v.buffer[7]); // se
		v.data[8] = adjacent(v.data[2],	Direction::w, v.buffer[8]); // sw

		return v;
	}

	[[deprecated]]
	HashVector nearbyCells(std::string_view hash) noexcept{
		HashVector v;

		v.size = 8;

		v.data[0] = adjacent(hash,	Direction::n, v.buffer[0]); // n
		v.data[1] = adjacent(hash,	Direction::s, v.buffer[1]); // s
		v.data[2] = adjacent(hash,	Direction::e, v.buffer[2]); // e
		v.data[3] = adjacent(hash,	Direction::w, v.buffer[3]); // w

		v.data[4] = adjacent(v.data[0],	Direction::e, v.buffer[4]); // ne
		v.data[5] = adjacent(v.data[0],	Direction::w, v.buffer[5]); // nw

		v.data[6] = adjacent(v.data[1],	Direction::e, v.buffer[6]); // se
		v.data[7] = adjacent(v.data[1],	Direction::w, v.buffer[7]); // sw

		return v;
	}
}



#if 0
	template<typename P>
	void searchNearby(double lat, double lon, double radius, P &&search){
		auto const cells = selectCells(lat, lon, radius);

		for(auto &cell : cells){
			auto &[first, last] = search(cell);

			for(auto it = first; it != last; ++it){
				auto const &[p_lat, p_lon] = search.lat_lon(it);

				auto const distance = distance(lat, lon, p_lat, p_lon);

				if (distance <= radius)
					search.push_back(it);

				if (!search.ok())
					return;
			}
		}
	}

	template<typename P>
	void searchNearby(Point p, double radius, P &&search){
		return searchNearby(p.lat, p.lon, radius, std::forward<P>(search));
	}
#endif
